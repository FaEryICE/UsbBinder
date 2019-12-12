// UsbBinder.cpp : �������̨Ӧ�ó������ڵ㡣
//
#include "stdafx.h"
#include <windows.h>
#include <initguid.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <string.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include <iostream>

#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"setupapi.lib")

using namespace std;

#define _USB_MODE_

typedef enum
{
    EnumDevContinue = 0,
    EnumDevAbort = 1,

} EnumDevState;

typedef struct _RV_DEV_INFO
{
    ULONG   ulDeviceType;
    ULONG   ulDeviceNumber;

    CHAR    szSerialNumber[50];
    CHAR    szVendorId[50];
    CHAR    szProductId[50];

} RvDevInfo, *PRvDevInfo;
//---------------------------------------------------------------------------------------------------------------
typedef ULONG(WINAPI* PFN_EnumDevInterfaceNotify)(LPSTR lpParentDevIdString, LPSTR lpDevicePath, PVOID lpContext);

ULONG WINAPI DevInterfaceEnumNotify(LPSTR lpParentDevIdString, LPSTR lpDevicePath, PVOID lpContext);
ULONG        EnumDispacthDevInterface(LPGUID lpGuid, PFN_EnumDevInterfaceNotify Notify, PVOID lpContext);
PVOID        GetCurrentRemovableMediaInfo();
//---------------------------------------------------------------------------------------------------------------

int main()
{
    PVOID lpContext = NULL;

#ifdef _USB_MODE_

    // �ŵ�U���в��ԣ����ȡ��ǰU����Ϣ
    lpContext = GetCurrentRemovableMediaInfo();

#endif

    // �ҵ���Ӧ�̷���Devʵ����Ϣ(VID PID SerialNumber�ȵ�)
    EnumDispacthDevInterface((LPGUID)&GUID_DEVINTERFACE_DISK, DevInterfaceEnumNotify, lpContext);

    if (lpContext)
    {
        delete lpContext;
        lpContext = NULL;
    }

    getchar();

    return 0;
}
//---------------------------------------------------------------------------------------------------------------
PVOID GetCurrentRemovableMediaInfo()
{
    HANDLE                     hDriver                   = INVALID_HANDLE_VALUE;
    BOOL                       bIoRet                    = FALSE;
    ULONG                      ulReturned                = 0;
    ULONG                      ulEnumState               = EnumDevContinue;
    STORAGE_DEVICE_NUMBER      sDiCeN                    = { 0 };
    STORAGE_PROPERTY_QUERY     SptyQuery;
    PSTORAGE_DEVICE_DESCRIPTOR lpDescriptor              = NULL;
    CHAR                       szCurDirectory[MAX_PATH]  = { 0 };
    CHAR                       szDevSymbolName[MAX_PATH] = "\\\\.\\A:";
    LPSTR                      lpszAttitube              = NULL;
    ULONG                      ulDriveType               = 0;
    PRvDevInfo                 lpRvDevInfo               = NULL;

    do
    {
        ulDriveType = GetDriveType(NULL);
        if (ulDriveType != DRIVE_REMOVABLE && ulDriveType != DRIVE_FIXED)
        {
            break;
        }

        GetCurrentDirectoryA(MAX_PATH, szCurDirectory);
        szDevSymbolName[4] = szCurDirectory[0];

        // ��������д���ԣ�����򿪴����豸�����ʧ��......
        hDriver = CreateFileA(szDevSymbolName,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (INVALID_HANDLE_VALUE == hDriver)
        {
            break;
        }

        bIoRet = DeviceIoControl(hDriver,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            NULL,
            0,
            &sDiCeN,
            sizeof(sDiCeN),
            &ulReturned,
            NULL);

        if (!bIoRet)
        {
            break;
        }

        // obtain device property
        SptyQuery.PropertyId = StorageDeviceProperty;
        SptyQuery.QueryType  = PropertyStandardQuery;

        // ���IOCtrol����Ҫ�Ȱ��ڴ�����ã������������᷵����ȷ�����ݳ���
        ulReturned = 300;
        lpDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR) new (std::nothrow) char[ulReturned];
        if (!lpDescriptor)
        {
            break;
        }

        RtlZeroMemory(lpDescriptor, ulReturned);
        lpDescriptor->Size = ulReturned;

        bIoRet = DeviceIoControl(hDriver,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &SptyQuery,
            sizeof(SptyQuery),
            lpDescriptor,
            ulReturned,
            &ulReturned,
            NULL);

        if (!bIoRet)
        {
            break;
        }

        lpRvDevInfo = new (std::nothrow) RvDevInfo;
        if (!lpRvDevInfo)
        {
            break;
        }

        RtlZeroMemory(lpRvDevInfo, sizeof(RvDevInfo));

        lpRvDevInfo->ulDeviceType = sDiCeN.DeviceType;
        lpRvDevInfo->ulDeviceNumber = sDiCeN.DeviceNumber;

        if (lpDescriptor->ProductIdOffset)
        {
            lpszAttitube = (LPSTR)((PBYTE)lpDescriptor + lpDescriptor->ProductIdOffset);
            StringCbCopyA(lpRvDevInfo->szProductId, sizeof(lpRvDevInfo->szProductId), lpszAttitube);
            printf("ProductId:%s\r\n", lpszAttitube);
        }

        if (lpDescriptor->VendorIdOffset)
        {
            lpszAttitube = (LPSTR)((PBYTE)lpDescriptor + lpDescriptor->VendorIdOffset);
            StringCbCopyA(lpRvDevInfo->szVendorId, sizeof(lpRvDevInfo->szVendorId), lpszAttitube);
            printf("VendorId:%s\r\n", lpszAttitube);
        }

        if (lpDescriptor->SerialNumberOffset)
        {
            lpszAttitube = (LPSTR)((PBYTE)lpDescriptor + lpDescriptor->SerialNumberOffset);
            StringCbCopyA(lpRvDevInfo->szSerialNumber, sizeof(lpRvDevInfo->szSerialNumber), lpszAttitube);
            printf("SerialNumber:%s\r\n", lpszAttitube);
        }

    } while (FALSE);

    if (lpDescriptor)
    {
        delete[] lpDescriptor;
        lpDescriptor = NULL;
    }

    if (INVALID_HANDLE_VALUE != hDriver)
    {
        CloseHandle(hDriver);
        hDriver = INVALID_HANDLE_VALUE;
    }

    return lpRvDevInfo;
}
//---------------------------------------------------------------------------------------------------------------
ULONG WINAPI DevInterfaceEnumNotify(LPSTR lpParentDevIdString, LPSTR lpDevicePath, PVOID lpContext)
{
    HANDLE                     hDriver       = INVALID_HANDLE_VALUE;
    BOOL                       bIoRet        = FALSE;
    ULONG                      ulReturned    = 0;
    ULONG                      ulEnumState   = EnumDevContinue;
    STORAGE_DEVICE_NUMBER      sDiCeN        = { 0 };
    STORAGE_PROPERTY_QUERY     SptyQuery;
    PSTORAGE_DEVICE_DESCRIPTOR lpDescriptor  = NULL;
    LPSTR                      lpszAttitube  = NULL;
    PRvDevInfo                 lpRvDevInfo   = (PRvDevInfo)lpContext;

    do
    {
        if (!lpContext)
        {
            break;
        }

        if (_strnicmp(lpParentDevIdString, "USB\\VID", strlen("USB\\VID")) != 0)
        {
            break;
        }

        hDriver = CreateFileA(lpDevicePath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            NULL,
            NULL);

        if (INVALID_HANDLE_VALUE == hDriver)
        {
            break;
        }

        // obtain DeviceNumber
        bIoRet = DeviceIoControl(hDriver,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            NULL,
            0,
            &sDiCeN,
            sizeof(sDiCeN),
            &ulReturned,
            NULL);

        if (!bIoRet)
        {
            break;
        }

        // ��IO�����Ȱ��ڴ�׼���ã���������������㷵���ڴ治�㣬Ҳ������㷵����ȷ��С
        ulReturned   = 300;
        lpDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR) new (std::nothrow) char[ulReturned];
        if (!lpDescriptor)
        {
            break;
        }

        RtlZeroMemory(lpDescriptor, ulReturned);
        lpDescriptor->Size = ulReturned;

        // obtain device property
        SptyQuery.PropertyId = StorageDeviceProperty;
        SptyQuery.QueryType  = PropertyStandardQuery;

        bIoRet = DeviceIoControl(hDriver,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &SptyQuery,
            sizeof(SptyQuery),
            lpDescriptor,
            ulReturned,
            &ulReturned,
            NULL);

        if (!bIoRet)
        {
            break;
        }

        // Compare to Target Usb Device
        if (sDiCeN.DeviceType != lpRvDevInfo->ulDeviceType ||
            sDiCeN.DeviceNumber != lpRvDevInfo->ulDeviceNumber)
        {
            break;
        }

        if (lpDescriptor->VendorIdOffset)
        {
            lpszAttitube = (LPSTR)((PBYTE)lpDescriptor + lpDescriptor->VendorIdOffset);
            if (_strnicmp(lpszAttitube, lpRvDevInfo->szVendorId, sizeof(lpRvDevInfo->szVendorId)) != 0)
            {
                break;
            }
        }

        if (lpDescriptor->ProductIdOffset)
        {
            lpszAttitube = (LPSTR)((PBYTE)lpDescriptor + lpDescriptor->ProductIdOffset);
            if (_strnicmp(lpszAttitube, lpRvDevInfo->szProductId, sizeof(lpRvDevInfo->szProductId)) != 0)
            {
                break;
            }
        }

        if (lpDescriptor->SerialNumberOffset)
        {
            lpszAttitube = (LPSTR)((PBYTE)lpDescriptor + lpDescriptor->SerialNumberOffset);
            if (_strnicmp(lpszAttitube, lpRvDevInfo->szSerialNumber, sizeof(lpRvDevInfo->szSerialNumber)) != 0)
            {
                break;
            }
        }

        DISK_GEOMETRY DiskGeometry;
        bIoRet = DeviceIoControl(hDriver,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL,
            0,
            &DiskGeometry,
            sizeof(DiskGeometry),
            NULL,
            NULL);

        if (bIoRet)
        {
            printf("[%s]     ��������: %I64d\n", __FUNCTION__, DiskGeometry.Cylinders.QuadPart);
            printf("[%s]     ��������: %d\n",    __FUNCTION__, DiskGeometry.MediaType);
            printf("[%s] ÿ����ŵ���: %d\n",    __FUNCTION__, DiskGeometry.TracksPerCylinder);
            printf("[%s] ÿ�ŵ�������: %d\n",    __FUNCTION__, DiskGeometry.SectorsPerTrack);
            printf("[%s] ÿ�����ֽ���: %d\n",    __FUNCTION__, DiskGeometry.BytesPerSector);
        }

        // --------VID + PID + SerialNumber--------
        // ��Щ����û���ϸ����ر�׼��PID SerialNumber������ô�ɿ������ܵ�������Щ����ʶ���U�̲�Ψһ
        // ��������ЩU��ûʲô���⣬Ҫ���ϸ����Ҫ�����ӵ�������Ϣ...
        printf("[%s] Hit Target USB\r\n", __FUNCTION__);
        printf("[%s] lpParentDevIdString: %s\r\n", __FUNCTION__, lpParentDevIdString);

        ulEnumState = EnumDevAbort;

    } while (FALSE);

    if (lpDescriptor)
    {
        delete[] lpDescriptor;
        lpDescriptor = NULL;
    }

    return ulEnumState;
}
//---------------------------------------------------------------------------------------------------------------
ULONG EnumDispacthDevInterface(LPGUID lpGuid, PFN_EnumDevInterfaceNotify Notify, PVOID lpContext)
{
    HDEVINFO                           hDevInfoSet        = NULL;
    SP_DEVICE_INTERFACE_DATA           ifdata             = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    SP_DEVINFO_DATA                    spdd               = { sizeof(SP_DEVINFO_DATA) };
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A pDetail            = NULL;
    BOOL                               bResult            = FALSE;
    BOOL                               bGetDetail         = FALSE;
    ULONG                              ulCount            = 0;
    ULONG                              ulDetailSize       = 0;
    ULONG                              ulNotifyCount      = 0;
    ULONG                              ulNotifyState      = EnumDevContinue;
    char                               chID[MAX_PATH * 2] = { 0 };

    hDevInfoSet = ::SetupDiGetClassDevs(lpGuid,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

    if (hDevInfoSet == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    ulDetailSize = 0;

    while (TRUE)
    {
        bResult = ::SetupDiEnumDeviceInterfaces(
            hDevInfoSet,
            NULL,
            lpGuid,
            ulCount,
            &ifdata);

        if (!bResult)
        {
            break;
        }

        bGetDetail = SetupDiGetInterfaceDeviceDetailA(
            hDevInfoSet,
            &ifdata,
            NULL,
            0,
            &ulDetailSize,
            NULL);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            break;
        }

        pDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A) new (std::nothrow) char[ulDetailSize];
        if (!pDetail)
        {
            break;
        }

        RtlZeroMemory(pDetail, ulDetailSize);
        pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        bGetDetail = SetupDiGetInterfaceDeviceDetailA(
            hDevInfoSet,
            &ifdata,
            pDetail,
            ulDetailSize,
            &ulDetailSize,
            &spdd);

        if (!bGetDetail)
        {
            break;
        }

        CM_Get_Parent(&spdd.DevInst, spdd.DevInst, 0);
        DWORD dwError = CM_Get_Device_IDA(spdd.DevInst, chID, sizeof(chID), 0);

        if (dwError == CR_SUCCESS)
        {
            _strupr_s(chID, sizeof(chID));

            ulNotifyState = Notify(chID, pDetail->DevicePath, lpContext);
            ulNotifyCount++;

            if (EnumDevAbort == ulNotifyState)
            {
                break;
            }
        }

        ulCount++;
        delete[] pDetail;
        pDetail = NULL;
    }

    ::SetupDiDestroyDeviceInfoList(hDevInfoSet);

    if (pDetail)
    {
        delete[] pDetail;
        pDetail = NULL;
    }

    return ulNotifyCount;
}
//---------------------------------------------------------------------------------------------------------------
