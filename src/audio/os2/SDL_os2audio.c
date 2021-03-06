/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_AUDIO_DRIVER_OS2

/* Allow access to a raw mixing buffer */

#include "../../core/os2/SDL_os2.h"

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_os2audio.h"

/* no capture devices so far, so need to init them  
   as soon as we have it, enable set OS2_HasCapture to 1 or remove the if */
#define OS2_HasCapture 0

/*
void lockIncr(volatile int *piVal);
#pragma aux lockIncr = \
" lock add [eax], 1 "\
parm [eax]; 

void lockDecr(volatile int *piVal);
#pragma aux lockDecr = \
" lock sub [eax], 1 "\
parm [eax];
*/


static ULONG _getEnvULong(PSZ pszName, ULONG ulMax, ULONG ulDefault)
{
  ULONG      ulValue;
  PCHAR      pcEnd;
  PSZ        pszEnvVal = SDL_getenv( pszName );

  if ( pszEnvVal == NULL )
    return ulDefault;

  ulValue = SDL_strtoul( (const char *)pszEnvVal, &pcEnd, 10 );
  return ( pcEnd == pszEnvVal ) || ( ulValue > ulMax ) ? ulDefault : ulMax;
}

static int _MCIError(PSZ pszFunc, ULONG ulResult)
{
  CHAR			acBuf[128];

  mciGetErrorString( ulResult, (PCHAR)&acBuf, sizeof(acBuf) );
  return SDL_SetError( "[%s] %s", pszFunc, &acBuf );
}

static void _mixIOError(PSZ pszFunction, ULONG ulRC)
{
  debug(SDL_LOG_CATEGORY_AUDIO, "%s() - failed, rc = 0x%X (%s)",
         pszFunction, ulRC,
         ulRC == MCIERR_INVALID_MODE ? "Mixer mode does not match request"
         : ulRC == MCIERR_INVALID_BUFFER ? "Caller sent an invalid buffer"
         : "unknown" );
}

LONG APIENTRY cbAudioWriteEvent(ULONG ulStatus, PMCI_MIX_BUFFER pBuffer,
                                ULONG ulFlags)
{
  PSDL_PrivateAudioData pAData = (PSDL_PrivateAudioData)pBuffer->ulUserParm;
  ULONG                 ulRC;

  if ( ulFlags != MIX_WRITE_COMPLETE )
  {
    debug(SDL_LOG_CATEGORY_AUDIO, "flags = 0x%X", ulFlags );
    return 0;
  }

//  lockDecr( (int *)&pAData->ulQueuedBuf );
  ulRC = DosPostEventSem( pAData->hevBuf );
  if ( ( ulRC != NO_ERROR ) && ( ulRC != ERROR_ALREADY_POSTED ) )
    debug(SDL_LOG_CATEGORY_AUDIO, "DosPostEventSem(), rc = %u", ulRC );

  return 1; // It seems, return value is not matter.
}

LONG APIENTRY cbAudioReadEvent(ULONG ulStatus, PMCI_MIX_BUFFER pBuffer,
                               ULONG ulFlags)
{
  PSDL_PrivateAudioData pAData = (PSDL_PrivateAudioData)pBuffer->ulUserParm;
  ULONG                 ulRC;

  if ( ulFlags != MIX_READ_COMPLETE )
  {
    debug(SDL_LOG_CATEGORY_AUDIO, "flags = 0x%X", ulFlags );
    return 0;
  }

  pAData->stMCIMixSetup.pmixRead( pAData->stMCIMixSetup.ulMixHandle, pBuffer,
                                  1 );

  ulRC = DosPostEventSem( pAData->hevBuf );
  if ( ( ulRC != NO_ERROR ) && ( ulRC != ERROR_ALREADY_POSTED ) )
    debug(SDL_LOG_CATEGORY_AUDIO, "DosPostEventSem(), rc = %u", ulRC );

  return 1;
}


static void OS2_DetectDevices(void)
{
  MCI_SYSINFO_PARMS      stMCISysInfo;
  CHAR                   acBuf[256];
  ULONG                  ulDevicesNum;
  MCI_SYSINFO_LOGDEVICE  stLogDevice;
  MCI_SYSINFO_PARMS      stSysInfoParams;
  ULONG                  ulRC;
  ULONG                  ulHandle = 0;

  acBuf[0] = '\0';
  stMCISysInfo.pszReturn    = &acBuf;
  stMCISysInfo.ulRetSize    = sizeof(acBuf);
  stMCISysInfo.usDeviceType = MCI_DEVTYPE_AUDIO_AMPMIX;
  ulRC = mciSendCommand( 0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_QUANTITY,
                         &stMCISysInfo, 0 );
  if ( ulRC != NO_ERROR )
  {
    debug(SDL_LOG_CATEGORY_AUDIO, "MCI_SYSINFO, MCI_SYSINFO_QUANTITY - failed, rc = 0x%X", ulRC );
    return;
  }

  ulDevicesNum = atol( stMCISysInfo.pszReturn );

  for( stSysInfoParams.ulNumber = 0; stSysInfoParams.ulNumber < ulDevicesNum;
       stSysInfoParams.ulNumber++ )
  {
    // Get device install name.
    stSysInfoParams.pszReturn    = &acBuf;
    stSysInfoParams.ulRetSize    = sizeof( acBuf );
    stSysInfoParams.usDeviceType = MCI_DEVTYPE_AUDIO_AMPMIX;
    ulRC = mciSendCommand( 0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_INSTALLNAME,
                           &stSysInfoParams, 0 );
    if ( ulRC != NO_ERROR )
    {
      debug(SDL_LOG_CATEGORY_AUDIO, "MCI_SYSINFO, MCI_SYSINFO_INSTALLNAME - failed, rc = 0x%X", ulRC );
      continue;
    }

    // Get textual product description.
    stSysInfoParams.ulItem = MCI_SYSINFO_QUERY_DRIVER;
    stSysInfoParams.pSysInfoParm = &stLogDevice;
    strcpy( &stLogDevice.szInstallName, stSysInfoParams.pszReturn );
    ulRC = mciSendCommand( 0, MCI_SYSINFO, MCI_WAIT | MCI_SYSINFO_ITEM,
                           &stSysInfoParams, 0 );
    if ( ulRC != NO_ERROR )
    {
      debug(SDL_LOG_CATEGORY_AUDIO, "MCI_SYSINFO, MCI_SYSINFO_ITEM - failed, rc = 0x%X", ulRC );
      continue;
    }

    ulHandle++;
    SDL_AddAudioDevice( 0, &stLogDevice.szProductInfo, (void *)(ulHandle) );
#if OS2_HasCapture
    ulHandle++;
    SDL_AddAudioDevice( 1, &stLogDevice.szProductInfo, (void *)(ulHandle) );
#endif
  }
}

static void OS2_WaitDevice(_THIS)
{
  PSDL_PrivateAudioData pAData = (PSDL_PrivateAudioData)this->hidden;
  ULONG                 ulRC;

  /* Wait for an audio chunk to finish */
  ulRC = DosWaitEventSem( pAData->hevBuf, 5000 );
  if ( ulRC != NO_ERROR )
    debug(SDL_LOG_CATEGORY_AUDIO, "DosWaitEventSem(), rc = %u", ulRC );
}

static Uint8 *OS2_GetDeviceBuf(_THIS)
{
  PSDL_PrivateAudioData pAData = (PSDL_PrivateAudioData)this->hidden;

  return pAData->aMixBuffers[pAData->ulNextBuf].pBuffer;
}

static void OS2_PlayDevice(_THIS)
{
  PSDL_PrivateAudioData pAData = (PSDL_PrivateAudioData)this->hidden;
  ULONG                 ulRC;
  PMCI_MIX_BUFFER       pMixBuffer = &pAData->aMixBuffers[pAData->ulNextBuf];

  /* Queue it up */
//  lockIncr( (int *)&pAData->ulQueuedBuf );
  ulRC = pAData->stMCIMixSetup.pmixWrite( pAData->stMCIMixSetup.ulMixHandle,
                                          pMixBuffer, 1 );
  if ( ulRC != MCIERR_SUCCESS )
    _mixIOError( "pmixWrite", ulRC );
  else
    pAData->ulNextBuf = (pAData->ulNextBuf + 1) % pAData->cMixBuffers;
}

static void OS2_WaitDone(_THIS)
{
  PSDL_PrivateAudioData pAData = (PSDL_PrivateAudioData)this->hidden;

  DosWaitEventSem( pAData->hevBuf, 3000 );
}

static void OS2_CloseDevice(_THIS)
{
  PSDL_PrivateAudioData pAData = (PSDL_PrivateAudioData)this->hidden;
  MCI_GENERIC_PARMS     sMCIGenericParms;
  ULONG                 ulRC;

  if ( pAData == NULL )
    return;

  /* Close up audio */

  if ( pAData->usDeviceId != (USHORT)~0 )
  {
    // Device is open.

    if ( pAData->stMCIMixSetup.ulBitsPerSample != 0 )
    {
      // Mixer was initialized.
      ulRC = mciSendCommand( pAData->usDeviceId, MCI_MIXSETUP,
                             MCI_WAIT | MCI_MIXSETUP_DEINIT,
                             &pAData->stMCIMixSetup, 0 );
      if ( ulRC != MCIERR_SUCCESS )
        debug(SDL_LOG_CATEGORY_AUDIO, "MCI_MIXSETUP, MCI_MIXSETUP_DEINIT - failed" );
    }

    if ( pAData->cMixBuffers != 0 )
    {
      // Buffers was allocated.
      MCI_BUFFER_PARMS             stMCIBuffer;

      stMCIBuffer.ulBufferSize = pAData->aMixBuffers[0].ulBufferLength;
      stMCIBuffer.ulNumBuffers = pAData->cMixBuffers;
      stMCIBuffer.pBufList = &pAData->aMixBuffers;

      ulRC = mciSendCommand( pAData->usDeviceId, MCI_BUFFER,
                             MCI_WAIT | MCI_DEALLOCATE_MEMORY, &stMCIBuffer, 0 );
      if ( ulRC != MCIERR_SUCCESS )
        debug(SDL_LOG_CATEGORY_AUDIO, "MCI_BUFFER, MCI_DEALLOCATE_MEMORY - failed" );
    }

    ulRC = mciSendCommand( pAData->usDeviceId, MCI_CLOSE, MCI_WAIT,
                           &sMCIGenericParms, 0 );
    if ( ulRC != MCIERR_SUCCESS )
      debug(SDL_LOG_CATEGORY_AUDIO, "MCI_CLOSE - failed" );
  }

  if ( pAData->hevBuf != NULLHANDLE )
    DosCloseEventSem( pAData->hevBuf );

  SDL_free( pAData );
  this->hidden = NULL;
}

static int OS2_OpenDevice(_THIS, void *handle, const char *devname,
                          int iscapture)
{
  PSDL_PrivateAudioData pAData;
  SDL_AudioFormat       SDLAudioFmt;
  MCI_AMP_OPEN_PARMS    stMCIAmpOpen = { 0 };
  MCI_BUFFER_PARMS      stMCIBuffer = { 0 };
  ULONG                 ulRC;
  ULONG                 ulIdx;

  for( SDLAudioFmt = SDL_FirstAudioFormat( this->spec.format );
       SDLAudioFmt != 0; SDLAudioFmt = SDL_NextAudioFormat() )
  {
    if ( ( SDLAudioFmt == AUDIO_U8 ) || ( SDLAudioFmt == AUDIO_S16 ) )
      break;
  }

  if ( SDLAudioFmt == 0 )
  {
    debug(SDL_LOG_CATEGORY_AUDIO, "Unsupported audio format, AUDIO_S16 used" );
    SDLAudioFmt = AUDIO_S16;
    this->spec.freq = AUDIO_S16;
  }

  pAData = SDL_calloc( 1, sizeof(SDL_PrivateAudioData) );
  if ( pAData == NULL )
    return SDL_OutOfMemory();
  this->hidden = pAData;

  ulRC = DosCreateEventSem( NULL, &pAData->hevBuf, DCE_AUTORESET, TRUE );
  if ( ulRC != NO_ERROR )
  {
    debug(SDL_LOG_CATEGORY_AUDIO, "DosCreateEventSem() failed, rc = %u", ulRC );
    return -1;
  }

  // Open audio device
  stMCIAmpOpen.usDeviceID = handle != NULL ? ( ((ULONG)handle) - 1 ) : 0;
  stMCIAmpOpen.pszDeviceType = (PSZ)MCI_DEVTYPE_AUDIO_AMPMIX;
  ulRC = mciSendCommand( 0, MCI_OPEN,
           _getEnvULong( "SDL_AUDIO_SHARE", 1, 0 ) != 0
             ? MCI_WAIT | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE
             : MCI_WAIT | MCI_OPEN_TYPE_ID,
           &stMCIAmpOpen,  0 );
  if ( ulRC != MCIERR_SUCCESS )
  {
    stMCIAmpOpen.usDeviceID = (USHORT)~0;
    OS2_CloseDevice( this );
    return _MCIError( "MCI_OPEN", ulRC );
  }
  pAData->usDeviceId = stMCIAmpOpen.usDeviceID;

  if ( iscapture != 0 )
  {
    MCI_CONNECTOR_PARMS  stMCIConnector = { 0 };
    MCI_AMP_SET_PARMS    stMCIAmpSet = { 0 };
    BOOL                 fLineIn = _getEnvULong( "SDL_AUDIO_LINEIN", 1, 0 );

    // Set particular connector.
    stMCIConnector.ulConnectorType = fLineIn ? MCI_LINE_IN_CONNECTOR
                                             : MCI_MICROPHONE_CONNECTOR;
    mciSendCommand( stMCIAmpOpen.usDeviceID, MCI_CONNECTOR,
                    MCI_WAIT | MCI_ENABLE_CONNECTOR |
                    MCI_CONNECTOR_TYPE, &stMCIConnector, 0 );

    // Disable monitor.
    stMCIAmpSet.ulItem = MCI_AMP_SET_MONITOR;
    mciSendCommand( stMCIAmpOpen.usDeviceID, MCI_SET,
                    MCI_WAIT | MCI_SET_OFF | MCI_SET_ITEM,
                    &stMCIAmpSet, 0 );


    // Set record volume.
    stMCIAmpSet.ulLevel = _getEnvULong( "SDL_AUDIO_RECVOL", 100, 90 );
    stMCIAmpSet.ulItem  = MCI_AMP_SET_AUDIO;
    stMCIAmpSet.ulAudio = MCI_SET_AUDIO_ALL; // Both cnannels.
    stMCIAmpSet.ulValue = fLineIn ? MCI_LINE_IN_CONNECTOR
                                  : MCI_MICROPHONE_CONNECTOR ;

    mciSendCommand( stMCIAmpOpen.usDeviceID, MCI_SET,
                    MCI_WAIT | MCI_SET_AUDIO | MCI_AMP_SET_GAIN,
                    &stMCIAmpSet, 0 );
  }

  this->spec.format = SDLAudioFmt;
  this->spec.channels = this->spec.channels > 1 ? 2 : 1;
  if ( this->spec.freq < 8000 )
    this->spec.freq = 8000;
  if ( this->spec.freq > 48000 )
    this->spec.freq = 48000;

  // Setup mixer.
  pAData->stMCIMixSetup.ulFormatTag     = MCI_WAVE_FORMAT_PCM;
  pAData->stMCIMixSetup.ulBitsPerSample = SDL_AUDIO_BITSIZE( SDLAudioFmt );
  pAData->stMCIMixSetup.ulSamplesPerSec = this->spec.freq;
  pAData->stMCIMixSetup.ulChannels      = this->spec.channels;
  pAData->stMCIMixSetup.ulDeviceType    = MCI_DEVTYPE_WAVEFORM_AUDIO;
  if ( iscapture == 0 )
  {
    pAData->stMCIMixSetup.ulFormatMode  = MCI_PLAY;
    pAData->stMCIMixSetup.pmixEvent     = cbAudioWriteEvent;
  }
  else
  {
    pAData->stMCIMixSetup.ulFormatMode  = MCI_RECORD;
    pAData->stMCIMixSetup.pmixEvent     = cbAudioReadEvent;
  }

  ulRC = mciSendCommand( pAData->usDeviceId, MCI_MIXSETUP,
                     MCI_WAIT | MCI_MIXSETUP_INIT, &pAData->stMCIMixSetup, 0 );
  if ( ( ulRC != MCIERR_SUCCESS ) && ( this->spec.freq > 44100 ) )
  {
    pAData->stMCIMixSetup.ulSamplesPerSec = 44100;
    this->spec.freq = 44100;
    ulRC = mciSendCommand( pAData->usDeviceId, MCI_MIXSETUP,
                     MCI_WAIT | MCI_MIXSETUP_INIT, &pAData->stMCIMixSetup, 0 );
  }
  
  debug(SDL_LOG_CATEGORY_AUDIO, "Setup mixer [BPS: %u, Freq.: %u, Channels: %u]: %s",
         pAData->stMCIMixSetup.ulBitsPerSample,
         pAData->stMCIMixSetup.ulSamplesPerSec,
         pAData->stMCIMixSetup.ulChannels,
         ulRC == MCIERR_SUCCESS ? "SUCCESS" : "FAIL" );

  if ( ulRC != MCIERR_SUCCESS )
  {
    pAData->stMCIMixSetup.ulBitsPerSample = 0;
    OS2_CloseDevice( this );
    return _MCIError( "MCI_MIXSETUP", ulRC );
  }

  this->spec.samples = pAData->stMCIMixSetup.ulSamplesPerSec;
  /* Update the fragment size as size in bytes */
  SDL_CalculateAudioSpec( &this->spec );

  // Allocate memory buffers

  stMCIBuffer.ulBufferSize = (this->spec.freq / 1000) * 100;// this->spec.size;
  stMCIBuffer.ulNumBuffers = NUM_BUFFERS;
  stMCIBuffer.pBufList     = &pAData->aMixBuffers;

  ulRC = mciSendCommand( pAData->usDeviceId, MCI_BUFFER,
                         MCI_WAIT | MCI_ALLOCATE_MEMORY, &stMCIBuffer, 0 );
  if ( ulRC != MCIERR_SUCCESS )
  {
    OS2_CloseDevice( this );
    return _MCIError( "MCI_BUFFER", ulRC );
  }
  pAData->cMixBuffers = stMCIBuffer.ulNumBuffers;
  this->spec.size = stMCIBuffer.ulBufferSize;

  // Fill all device buffers with data

  for( ulIdx = 0; ulIdx < stMCIBuffer.ulNumBuffers; ulIdx++ )
  {
    pAData->aMixBuffers[ulIdx].ulFlags        = 0;
    pAData->aMixBuffers[ulIdx].ulBufferLength = stMCIBuffer.ulBufferSize;
    pAData->aMixBuffers[ulIdx].ulUserParm     = (ULONG)pAData;

    memset( ((PMCI_MIX_BUFFER)stMCIBuffer.pBufList)[ulIdx].pBuffer,
            this->spec.silence, stMCIBuffer.ulBufferSize );
  }

  // Write buffers to kick off the amp mixer
//  pAData->ulQueuedBuf = 1;//stMCIBuffer.ulNumBuffers;
  ulRC = pAData->stMCIMixSetup.pmixWrite( pAData->stMCIMixSetup.ulMixHandle,
                                          &pAData->aMixBuffers,
                                          1 );//stMCIBuffer.ulNumBuffers );
  if ( ulRC != MCIERR_SUCCESS )
  {
    _mixIOError( "pmixWrite", ulRC );
    return -1;
  }

  return 0;
}


static int OS2_Init(SDL_AudioDriverImpl * impl)
{
  /* Set the function pointers */
  impl->DetectDevices = OS2_DetectDevices;
  impl->OpenDevice    = OS2_OpenDevice;
  impl->PlayDevice    = OS2_PlayDevice;
  impl->WaitDevice    = OS2_WaitDevice;
//gone in 2.0.12   impl->WaitDone      = OS2_WaitDone;
  impl->GetDeviceBuf  = OS2_GetDeviceBuf;
  impl->CloseDevice   = OS2_CloseDevice;

#if OS2_HasCapture
  impl->HasCaptureSupport = SDL_TRUE;
#endif

  return 1;   /* this audio target is available. */
}


AudioBootStrap OS2AUDIO_bootstrap = { "MMOS2", "OS/2 DART", OS2_Init, 0 };

#endif /* SDL_AUDIO_DRIVER_OS2 */

/* vi: set ts=4 sw=4 expandtab: */
