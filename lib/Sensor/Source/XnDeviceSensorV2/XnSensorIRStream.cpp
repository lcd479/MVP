/*****************************************************************************
*                                                                            *
*  PrimeSense Sensor 5.x Alpha                                               *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of PrimeSense Sensor                                    *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "XnDeviceSensorInit.h"
#include "XnSensorIRStream.h"
#include "XnIRProcessor.h"
#include <XnOS.h>
#include "XnCmosInfo.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define XN_IR_MAX_BUFFER_SIZE					(XN_SXGA_X_RES * XN_SXGA_Y_RES * sizeof(XnRGB24Pixel))

//---------------------------------------------------------------------------
// XnSensorIRStream class
//---------------------------------------------------------------------------
XnSensorIRStream::XnSensorIRStream(const XnChar* StreamName, XnSensorObjects* pObjects) : 
	XnIRStream(StreamName, FALSE),
	m_Helper(pObjects),
	m_InputFormat(XN_STREAM_PROPERTY_INPUT_FORMAT, 0),
	m_CroppingMode(XN_STREAM_PROPERTY_CROPPING_MODE, XN_CROPPING_MODE_NORMAL),
	m_FirmwareCropSizeX("FirmwareCropSizeX", 0, StreamName),
	m_FirmwareCropSizeY("FirmwareCropSizeY", 0, StreamName),
	m_FirmwareCropOffsetX("FirmwareCropOffsetX", 0, StreamName),
	m_FirmwareCropOffsetY("FirmwareCropOffsetY", 0, StreamName),
	m_FirmwareCropMode("FirmwareCropMode", XN_FIRMWARE_CROPPING_MODE_DISABLED, StreamName),
	m_ActualRead(XN_STREAM_PROPERTY_ACTUAL_READ_DATA, FALSE)
{
	m_ActualRead.UpdateSetCallback(SetActualReadCallback, this);
	m_CroppingMode.UpdateSetCallback(SetCroppingModeCallback, this);
}

XnStatus XnSensorIRStream::Init()
{
	XnStatus nRetVal = XN_STATUS_OK;

// 	nRetVal = SetBufferPool(&m_BufferPool);
// 	XN_IS_STATUS_OK(nRetVal);

	// init base
	nRetVal = XnIRStream::Init();
	XN_IS_STATUS_OK(nRetVal);

	// add properties
	XN_VALIDATE_ADD_PROPERTIES(this, &m_InputFormat, &m_ActualRead, &m_CroppingMode);

	// set base properties default values
	nRetVal = ResolutionProperty().UnsafeUpdateValue(XN_IR_STREAM_DEFAULT_RESOLUTION);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = FPSProperty().UnsafeUpdateValue(XN_IR_STREAM_DEFAULT_FPS);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = OutputFormatProperty().UnsafeUpdateValue(XN_IR_STREAM_DEFAULT_OUTPUT_FORMAT);
	XN_IS_STATUS_OK(nRetVal);

	// init helper
	nRetVal = m_Helper.Init(this, this);
	XN_IS_STATUS_OK(nRetVal);

	// register supported modes
	XnCmosPreset* pSupportedModes = m_Helper.GetPrivateData()->FWInfo.irModes.GetData();
	XnUInt8 nSupportedModes = m_Helper.GetPrivateData()->FWInfo.irModes.GetSize();
	nRetVal = AddSupportedModes(pSupportedModes, nSupportedModes);
	XN_IS_STATUS_OK(nRetVal);

	// data processor
	nRetVal = m_Helper.RegisterDataProcessorProperty(ResolutionProperty());
	XN_IS_STATUS_OK(nRetVal);

	// register for mirror
	XnCallbackHandle hCallbackDummy;
	nRetVal = IsMirroredProperty().OnChangeEvent().Register(IsMirroredChangedCallback, this, hCallbackDummy);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::Free()
{
	m_Helper.Free();
	XnIRStream::Free();
	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::MapPropertiesToFirmware()
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.MapFirmwareProperty(ResolutionProperty(), GetFirmwareParams()->m_IRResolution, FALSE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(FPSProperty(), GetFirmwareParams()->m_IRFPS, FALSE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropSizeX, GetFirmwareParams()->m_IRCropSizeX, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropSizeY, GetFirmwareParams()->m_IRCropSizeY, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropOffsetX, GetFirmwareParams()->m_IRCropOffsetX, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropOffsetY, GetFirmwareParams()->m_IRCropOffsetY, TRUE);
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.MapFirmwareProperty(m_FirmwareCropMode, GetFirmwareParams()->m_IRCropMode, TRUE);
	XN_IS_STATUS_OK(nRetVal);;

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::ConfigureStreamImpl()
{
	XnStatus nRetVal = XN_STATUS_OK;

	xnUSBShutdownReadThread(GetHelper()->GetPrivateData()->pSpecificImageUsb->pUsbConnection->UsbEp);

	nRetVal = SetActualRead(TRUE);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.ConfigureFirmware(ResolutionProperty());
	XN_IS_STATUS_OK(nRetVal);;
	nRetVal = m_Helper.ConfigureFirmware(FPSProperty());
	XN_IS_STATUS_OK(nRetVal);;

	// IR mirror is always off in firmware
	nRetVal = GetFirmwareParams()->m_IRMirror.SetValue(FALSE);
	XN_IS_STATUS_OK(nRetVal);

	// CMOS
	if (GetResolution() != XN_RESOLUTION_SXGA)
	{
		nRetVal = m_Helper.GetCmosInfo()->SetCmosConfig(XN_CMOS_TYPE_DEPTH, GetResolution(), GetFPS());
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::SetActualRead(XnBool bRead)
{
	XnStatus nRetVal = XN_STATUS_OK;

	if (m_ActualRead.GetValue() != bRead)
	{
		if (bRead)
		{
			xnLogVerbose(XN_MASK_DEVICE_SENSOR, "Creating USB IR read thread...");
			XnSpecificUsbDevice* pUSB = GetHelper()->GetPrivateData()->pSpecificImageUsb;
			nRetVal = xnUSBInitReadThread(pUSB->pUsbConnection->UsbEp, pUSB->nChunkReadBytes, XN_SENSOR_USB_DEPTH_BUFFERS, pUSB->nTimeout, XnDeviceSensorProtocolUsbEpCb, pUSB);
			XN_IS_STATUS_OK(nRetVal);
		}
		else
		{
			xnLogVerbose(XN_MASK_DEVICE_SENSOR, "Shutting down IR image read thread...");
			xnUSBShutdownReadThread(GetHelper()->GetPrivateData()->pSpecificImageUsb->pUsbConnection->UsbEp);
		}

		nRetVal = m_ActualRead.UnsafeUpdateValue(bRead);
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::OpenStreamImpl()
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = GetFirmwareParams()->m_Stream0Mode.SetValue(XN_VIDEO_STREAM_IR);
	XN_IS_STATUS_OK(nRetVal);

	// Cropping
	if (m_FirmwareCropMode.GetValue() != XN_FIRMWARE_CROPPING_MODE_DISABLED)
	{
		nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropSizeX);
		XN_IS_STATUS_OK(nRetVal);;
		nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropSizeY);
		XN_IS_STATUS_OK(nRetVal);;
		nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropOffsetX);
		XN_IS_STATUS_OK(nRetVal);;
		nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropOffsetY);
		XN_IS_STATUS_OK(nRetVal);;
	}
	nRetVal = m_Helper.ConfigureFirmware(m_FirmwareCropMode);
	XN_IS_STATUS_OK(nRetVal);;


	nRetVal = XnIRStream::Open();
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}


XnStatus XnSensorIRStream::CloseStreamImpl()
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = GetFirmwareParams()->m_Stream0Mode.SetValue(XN_VIDEO_STREAM_OFF);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnIRStream::Close();
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = SetActualRead(FALSE);
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::SetOutputFormat(XnOutputFormats nOutputFormat)
{
	XnStatus nRetVal = XN_STATUS_OK;

	switch (nOutputFormat)
	{
	case XN_OUTPUT_FORMAT_RGB24:
	case XN_OUTPUT_FORMAT_GRAYSCALE16:
		break;
	default:
		XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Unsupported IR output format: %d", nOutputFormat);
	}

	nRetVal = m_Helper.BeforeSettingDataProcessorProperty();
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnIRStream::SetOutputFormat(nOutputFormat);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.AfterSettingDataProcessorProperty();
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::SetFPS(XnUInt32 nFPS)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.BeforeSettingFirmwareParam(FPSProperty(), (XnUInt16)nFPS);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnIRStream::SetFPS(nFPS);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.AfterSettingFirmwareParam(FPSProperty());
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::SetResolution(XnResolutions nResolution)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = m_Helper.BeforeSettingFirmwareParam(ResolutionProperty(), (XnUInt16)nResolution);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = XnIRStream::SetResolution(nResolution);
	XN_IS_STATUS_OK(nRetVal);

	nRetVal = m_Helper.AfterSettingFirmwareParam(ResolutionProperty());
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::SetCroppingImpl(const XnCropping* pCropping, XnCroppingMode mode)
{
	XnStatus nRetVal = XN_STATUS_OK;

	XnFirmwareCroppingMode firmwareMode = m_Helper.GetFirmwareCroppingMode(mode, pCropping->bEnabled);

	nRetVal = ValidateCropping(pCropping);
	XN_IS_STATUS_OK(nRetVal);

	xnOSEnterCriticalSection(GetLock());

	if (m_Helper.GetFirmwareVersion() > XN_SENSOR_FW_VER_3_0)
	{
		nRetVal = m_Helper.StartFirmwareTransaction();
		if (nRetVal != XN_STATUS_OK)
		{
			xnOSLeaveCriticalSection(GetLock());
			return (nRetVal);
		}

		// mirror is done by software (meaning AFTER cropping, which is bad). So we need to flip the cropping area
		// to match requested area.
		XnUInt16 nXOffset = pCropping->nXOffset;
		if (IsMirrored())
		{
			nXOffset = (XnUInt16)(GetXRes() - pCropping->nXOffset - pCropping->nXSize);
		}

		if (pCropping->bEnabled)
		{
			nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropSizeX, pCropping->nXSize);

			if (nRetVal == XN_STATUS_OK)
				nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropSizeY, pCropping->nYSize);

			if (nRetVal == XN_STATUS_OK)
				nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropOffsetX, nXOffset);

			if (nRetVal == XN_STATUS_OK)
				nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropOffsetY, pCropping->nYOffset);
		}

		if (nRetVal == XN_STATUS_OK)
		{
			nRetVal = m_Helper.SimpleSetFirmwareParam(m_FirmwareCropMode, (XnUInt16)firmwareMode);
		}

		if (nRetVal != XN_STATUS_OK)
		{
			m_Helper.RollbackFirmwareTransaction();
			m_Helper.UpdateFromFirmware(m_FirmwareCropMode);
			m_Helper.UpdateFromFirmware(m_FirmwareCropOffsetX);
			m_Helper.UpdateFromFirmware(m_FirmwareCropOffsetY);
			m_Helper.UpdateFromFirmware(m_FirmwareCropSizeX);
			m_Helper.UpdateFromFirmware(m_FirmwareCropSizeY);
			xnOSLeaveCriticalSection(GetLock());
			return (nRetVal);
		}

		nRetVal = m_Helper.CommitFirmwareTransactionAsBatch();
		if (nRetVal != XN_STATUS_OK)
		{
			m_Helper.UpdateFromFirmware(m_FirmwareCropMode);
			m_Helper.UpdateFromFirmware(m_FirmwareCropOffsetX);
			m_Helper.UpdateFromFirmware(m_FirmwareCropOffsetY);
			m_Helper.UpdateFromFirmware(m_FirmwareCropSizeX);
			m_Helper.UpdateFromFirmware(m_FirmwareCropSizeY);
			xnOSLeaveCriticalSection(GetLock());
			return (nRetVal);
		}
	}

	nRetVal = m_CroppingMode.UnsafeUpdateValue(mode);
	XN_ASSERT(nRetVal == XN_STATUS_OK);

	nRetVal = XnIRStream::SetCropping(pCropping);


	xnOSLeaveCriticalSection(GetLock());
	XN_IS_STATUS_OK(nRetVal);

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::SetCropping(const XnCropping* pCropping)
{
	return SetCroppingImpl(pCropping, (XnCroppingMode)m_CroppingMode.GetValue());
}

XnStatus XnSensorIRStream::SetCroppingMode(XnCroppingMode mode)
{
	XnStatus nRetVal = XN_STATUS_OK;

	switch (mode)
	{
	case XN_CROPPING_MODE_NORMAL:
	case XN_CROPPING_MODE_INCREASED_FPS:
	case XN_CROPPING_MODE_SOFTWARE_ONLY:
		break;
	default:
		XN_LOG_WARNING_RETURN(XN_STATUS_DEVICE_BAD_PARAM, XN_MASK_DEVICE_SENSOR, "Bad cropping mode: %u", mode);
	}

	return SetCroppingImpl(GetCropping(), mode);
}

XnStatus XnSensorIRStream::CalcRequiredSize(XnUInt32* pnRequiredSize) const
{
	// in IR, in all resolutions except SXGA, we get additional 8 lines
	XnUInt32 nYRes = GetYRes();
	if (GetResolution() != XN_RESOLUTION_SXGA)
	{
		nYRes += 8;
	}

	*pnRequiredSize = GetXRes() * nYRes * GetBytesPerPixel();
	return XN_STATUS_OK;
}

XnStatus XnSensorIRStream::ReallocTripleFrameBuffer()
{
	XnStatus nRetVal = XN_STATUS_OK;

	if (IsOpen())
	{
		// before actually replacing buffer, lock the processor (so it will not continue to 
		// use old buffer)
		nRetVal = m_Helper.GetFirmware()->GetStreams()->LockStreamProcessor(GetType(), this);
		XN_IS_STATUS_OK(nRetVal);
	}

	nRetVal = XnIRStream::ReallocTripleFrameBuffer();
	if (nRetVal != XN_STATUS_OK)
	{
		m_Helper.GetFirmware()->GetStreams()->UnlockStreamProcessor(GetType(), this);
		return (nRetVal);
	}

	if (IsOpen())
	{
		nRetVal = m_Helper.GetFirmware()->GetStreams()->UnlockStreamProcessor(GetType(), this);
		XN_IS_STATUS_OK(nRetVal);
	}

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::CropImpl(XnStreamData* pStreamOutput, const XnCropping* pCropping)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	// if firmware cropping is disabled, crop
	if (m_FirmwareCropMode.GetValue() == XN_FIRMWARE_CROPPING_MODE_DISABLED)
	{
		nRetVal = XnIRStream::CropImpl(pStreamOutput, pCropping);
		XN_IS_STATUS_OK(nRetVal);
	}
	
	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::CreateDataProcessor(XnDataProcessor** ppProcessor)
{
	XnStatus nRetVal = XN_STATUS_OK;

	XnFrameBufferManager* pBufferManager;
	nRetVal = GetTripleBuffer(&pBufferManager);
	XN_IS_STATUS_OK(nRetVal);

	XnDataProcessor* pNew;
	XN_VALIDATE_NEW_AND_INIT(pNew, XnIRProcessor, this, &m_Helper, pBufferManager);

	*ppProcessor = pNew;

	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::OnIsMirroredChanged()
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	// if cropping is on, we need to flip it
	XnCropping cropping = *GetCropping();
	if (cropping.bEnabled)
	{
		nRetVal = SetCropping(&cropping);
		XN_IS_STATUS_OK(nRetVal);
	}
	
	return (XN_STATUS_OK);
}

XnStatus XnSensorIRStream::IsMirroredChangedCallback(const XnProperty* /*pSender*/, void* pCookie)
{
	XnSensorIRStream* pThis = (XnSensorIRStream*)pCookie;
	return pThis->OnIsMirroredChanged();
}

XnStatus XN_CALLBACK_TYPE XnSensorIRStream::SetActualReadCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorIRStream* pThis = (XnSensorIRStream*)pCookie;
	return pThis->SetActualRead(nValue == TRUE);
}

XnStatus XN_CALLBACK_TYPE XnSensorIRStream::SetCroppingModeCallback(XnActualIntProperty* /*pSender*/, XnUInt64 nValue, void* pCookie)
{
	XnSensorIRStream* pStream = (XnSensorIRStream*)pCookie;
	return pStream->SetCroppingMode((XnCroppingMode)nValue);
}

