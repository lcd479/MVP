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
#ifndef __XN_STREAM_READER_STREAM_H__
#define __XN_STREAM_READER_STREAM_H__

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "XnDeviceStream.h"

//---------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------
class XN_DDK_CPP_API XnStreamReaderStream : public XnDeviceStream
{
public:
	XnStreamReaderStream(const XnChar* strType, const XnChar* strName);
	~XnStreamReaderStream();

	XnStatus Init();
	XnStatus Free();

	inline XnStreamData* GetStreamData() { return m_pLastData; }
	virtual void NewDataAvailable(XnUInt64 nTimestamp, XnUInt32 nFrameID);
	void ReMarkDataAsNew();
	void Reset();

protected:
	XnStatus WriteImpl(XnStreamData* /*pStreamData*/) { return XN_STATUS_IO_DEVICE_FUNCTION_NOT_SUPPORTED; }
	XnStatus ReadImpl(XnStreamData* pStreamOutput);
	XnStatus Mirror(XnStreamData* /*pStreamData*/) const { return XN_STATUS_OK; }

	XnStatus CalcRequiredSize(XnUInt32* pnRequiredSize) const;

private:
	XnStatus OnRequiredSizeChanged();

	static XnStatus XN_CALLBACK_TYPE RequiredSizeChangedCallback(const XnProperty* pSender, void* pCookie);

	XnStreamData* m_pLastData;
	XnUInt32 m_nLastFrameIDFromStream;
};

#endif //__XN_STREAM_READER_STREAM_H__
