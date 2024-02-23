#include "process_manager.hpp"
#include <string>

#include <Psapi.h>

#include <TlHelp32.h>
#include <iostream>



// macros we use. Some can be found in wintrnl.h
#define SeDebugPriv 20
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)
#define NtCurrentProcess ( (HANDLE)(LONG_PTR) -1 ) 
#define ProcessHandleType 0x7
#define SystemHandleInformation 16

/*
STRUCTURES NEEDED FOR NTOPENPROCESS:
*/
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWCH   Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG           Length;
	HANDLE          RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG           Attributes;
	PVOID           SecurityDescriptor;
	PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID
{
	PVOID UniqueProcess;
	PVOID UniqueThread;
} CLIENT_ID, * PCLIENT_ID;

/*
STRUCTURES NEEDED FOR HANDLE INFORMATION:
*/

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
	ULONG ProcessId;
	BYTE ObjectTypeNumber;
	BYTE Flags;
	USHORT Handle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, * PSYSTEM_HANDLE; //i shortened it to SYSTEM_HANDLE for the sake of typing

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG HandleCount;
	SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

/*
FUNCTION PROTOTYPES:
*/
typedef NTSTATUS(NTAPI* _NtDuplicateObject)(
	HANDLE SourceProcessHandle,
	HANDLE SourceHandle,
	HANDLE TargetProcessHandle,
	PHANDLE TargetHandle,
	ACCESS_MASK DesiredAccess,
	ULONG Attributes,
	ULONG Options
	);

typedef NTSTATUS(NTAPI* _RtlAdjustPrivilege)(
	ULONG Privilege,
	BOOLEAN Enable,
	BOOLEAN CurrentThread,
	PBOOLEAN Enabled
	);

typedef NTSYSAPI NTSTATUS(NTAPI* _NtOpenProcess)(
	PHANDLE            ProcessHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID         ClientId
	);

typedef NTSTATUS(NTAPI* _NtQuerySystemInformation)(
	ULONG SystemInformationClass, //your supposed to supply the whole class but microsoft kept the enum mostly empty so I just passed 16 instead for handle info. Thats why you get a warning in your code btw
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
	);

SYSTEM_HANDLE_INFORMATION* hInfo; //holds the handle information

//the handles we will need to use later on

namespace hj
{
	HANDLE procHandle = NULL;
	HANDLE hProcess = NULL;
	HANDLE HijackedHandle = NULL;

	// simple function i made that will just initialize our Object_Attributes structure as NtOpenProcess will fail otherwise
	OBJECT_ATTRIBUTES InitObjectAttributes(PUNICODE_STRING name, ULONG attributes, HANDLE hRoot, PSECURITY_DESCRIPTOR security)
	{
		OBJECT_ATTRIBUTES object;

		object.Length = sizeof(OBJECT_ATTRIBUTES);
		object.ObjectName = name;
		object.Attributes = attributes;
		object.RootDirectory = hRoot;
		object.SecurityDescriptor = security;

		return object;
	}

	bool IsHandleValid(HANDLE handle) // i made this to simply check if a handle is valid rather than repeating the if statments
	{
		if (handle && handle != INVALID_HANDLE_VALUE)
			return true;
		else
			return false;

	}

	HANDLE HijackExistingHandle(DWORD dwTargetProcessId)
	{
		HMODULE Ntdll = GetModuleHandleA("ntdll"); // get the base address of ntdll.dll

		//get the address of RtlAdjustPrivilege in ntdll.dll so we can grant our process the highest permission possible
		_RtlAdjustPrivilege RtlAdjustPrivilege = (_RtlAdjustPrivilege)GetProcAddress(Ntdll, "RtlAdjustPrivilege");

		boolean OldPriv; //store the old privileges

		// Give our program SeDeugPrivileges whcih allows us to get a handle to every process, even the highest privileged SYSTEM level processes.
		RtlAdjustPrivilege(SeDebugPriv, TRUE, FALSE, &OldPriv);

		//get the address of NtQuerySystemInformation in ntdll.dll so we can find all the open handles on our system
		_NtQuerySystemInformation NtQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(Ntdll, "NtQuerySystemInformation");

		//get the address of NtDuplicateObject in ntdll.dll so we can duplicate an existing handle into our cheat, basically performing the hijacking
		_NtDuplicateObject NtDuplicateObject = (_NtDuplicateObject)GetProcAddress(Ntdll, "NtDuplicateObject");

		//get the address of NtOpenProcess in ntdll.dll so wecan create a Duplicate handle
		_NtOpenProcess NtOpenProcess = (_NtOpenProcess)GetProcAddress(Ntdll, "NtOpenProcess");


		//initialize the Object Attributes structure, you can just set each member to NULL rather than create a function like i did
		OBJECT_ATTRIBUTES Obj_Attribute = InitObjectAttributes(NULL, NULL, NULL, NULL);

		//clientID is a PDWORD or DWORD* of the process id to create a handle to
		CLIENT_ID clientID = { 0 };


		//the size variable is the amount of bytes allocated to store all the open handles
		DWORD size = sizeof(SYSTEM_HANDLE_INFORMATION);

		//we allocate the memory to store all the handles on the heap rather than the stack becuase of the large amount of data
		hInfo = (SYSTEM_HANDLE_INFORMATION*) new byte[size];

		//zero the memory handle info
		ZeroMemory(hInfo, size);

		//we use this for checking if the Native functions succeed
		NTSTATUS NtRet = NULL;

		do
		{
			// delete the previously allocated memory on the heap because it wasn't large enough to store all the handles
			delete[] hInfo;

			//increase the amount of memory allocated by 50%
			size *= 1.5;
			try
			{
				//set and allocate the larger size on the heap
				hInfo = (PSYSTEM_HANDLE_INFORMATION) new byte[size];
			}
			catch (std::bad_alloc) //catch a bad heap allocation.
			{
				procHandle ? CloseHandle(procHandle) : 0;
			}
			Sleep(1); //sleep for the cpu

			//we continue this loop until all the handles have been stored
		} while ((NtRet = NtQuerySystemInformation(SystemHandleInformation, hInfo, size, NULL)) == STATUS_INFO_LENGTH_MISMATCH);

		//check if we got all the open handles on our system
		if (!NT_SUCCESS(NtRet))
			procHandle ? CloseHandle(procHandle) : 0;


		//loop through each handle on our system, and filter out handles that are invalid or cant be hijacked
		for (unsigned int i = 0; i < hInfo->HandleCount; ++i)
		{
			//a variable to store the number of handles OUR cheat has open.
			static DWORD NumOfOpenHandles;

			//get the amount of outgoing handles OUR cheat has open
			GetProcessHandleCount(GetCurrentProcess(), &NumOfOpenHandles);

			//you can do a higher number if this is triggering false positives. Its just to make sure we dont fuck up and create thousands of handles
			if (NumOfOpenHandles > 500)
				procHandle ? CloseHandle(procHandle) : 0;

			//check if the current handle is valid, otherwise increment i and check the next handle
			if (!IsHandleValid((HANDLE)hInfo->Handles[i].Handle))
				continue;

			//check the handle type is 0x7 meaning a process handle so we dont hijack a file handle for example
			if (hInfo->Handles[i].ObjectTypeNumber != ProcessHandleType)
				continue;

			//set clientID to a pointer to the process with the handle to out target
			clientID.UniqueProcess = (DWORD*)hInfo->Handles[i].ProcessId;

			//if procHandle is open, close it
			procHandle ? CloseHandle(procHandle) : 0;

			//create a a handle with duplicate only permissions to the process with a handle to our target. NOT OUR TARGET.
			NtRet = NtOpenProcess(&procHandle, PROCESS_DUP_HANDLE, &Obj_Attribute, &clientID);
			if (!IsHandleValid(procHandle) || !NT_SUCCESS(NtRet)) //check is the funcions succeeded and check the handle is valid
				continue;

			//we duplicate the handle another process has to our target into our cheat with whatever permissions we want. I did all access.
			NtRet = NtDuplicateObject(procHandle, (HANDLE)hInfo->Handles[i].Handle, NtCurrentProcess, &HijackedHandle, PROCESS_ALL_ACCESS, 0, 0);
			if (!IsHandleValid(HijackedHandle) || !NT_SUCCESS(NtRet))//check is the funcions succeeded and check the handle is valid
				continue;

			//get the process id of the handle we duplicated and check its to our target
			if (GetProcessId(HijackedHandle) != dwTargetProcessId) {
				CloseHandle(HijackedHandle);
				continue;
			}

			hProcess = HijackedHandle;

			break;
		}

		procHandle ? CloseHandle(procHandle) : 0;

		return hProcess;
	}
}

StatusCode ProcessManager::attach(std::string ProcessName) {



	ProcessID = this->get_process_id(ProcessName);
	_is_invalid(ProcessID, FAILE_PROCESSID);

	hProcess = hj::HijackExistingHandle(ProcessID);
	_is_invalid(hProcess, FAILE_HPROCESS);

	ModuleAddress = reinterpret_cast<DWORD64>(this->get_process_module_handle(ProcessName));
	_is_invalid(ModuleAddress, FAILE_MODULE);

	Attached = true;

	return SUCCEED;
}

inline const DWORD BLOCKMAXSIZE = 409600;

int GetSignatureArray( const std::string& Signature, std::vector<WORD>& SignatureArray ) {
	std::string Sig = Signature;
	Sig.erase( std::remove( Sig.begin( ), Sig.end( ), ' ' ), Sig.end( ) );

	std::size_t Size = Sig.size( );

	if ( Size % 2 != 0 )
		return 0;

	for ( int i = 0; i < Size; i += 2 ) {
		std::string ByteString = Sig.substr( i, 2 );
		WORD Byte = ( ByteString == "??" ) ? 256 : std::stoi( ByteString, nullptr, 16 );
		SignatureArray.push_back( Byte );
	}
	return SignatureArray.size( );
}

void GetNextArray( std::vector<short>& NextArray, const std::vector<WORD>& SignatureArray ) {
	std::size_t Size = SignatureArray.size( );
	for ( int i = 0; i < Size; i++ )
		NextArray[ SignatureArray[ i ] ] = i;
}

void SearchMemoryBlock( byte* MemoryBuffer, const std::vector<short>& NextArray, const std::vector<WORD>& SignatureArray, DWORD64 StartAddress, DWORD Size, std::vector<DWORD64>& ResultArray ) {
	if ( !_proc_manager.read_memory( StartAddress, *MemoryBuffer, Size ) )
		return;

	int SignatureLength = SignatureArray.size( );

	for ( int i = 0, j, k; i < Size;) {
		j = i; k = 0;

		for ( ; k < SignatureLength && j < Size && ( SignatureArray[ k ] == MemoryBuffer[ j ] || SignatureArray[ k ] == 256 ); k++, j++ );

		if ( k == SignatureLength )
			ResultArray.push_back( StartAddress + i );

		if ( ( i + SignatureLength ) >= Size )
			return;

		int Num = NextArray[ MemoryBuffer[ i + SignatureLength ] ];
		if ( Num == -1 )
			i += ( SignatureLength - NextArray[ 256 ] );
		else
			i += ( SignatureLength - Num );
	}
}

std::vector<DWORD64> ProcessManager::search_memory( const std::string& Signature, DWORD64 StartAddress, DWORD64 EndAddress, int SearchNum ) {
	std::vector<DWORD64> ResultArray;
	std::vector<WORD> SignatureArray;
	std::vector<short> NextArray( 260, -1 );

	byte* MemoryBuffer = new byte[ BLOCKMAXSIZE ];

	if ( GetSignatureArray( Signature, SignatureArray ) <= 0 )
		return ResultArray;

	GetNextArray( NextArray, SignatureArray );

	MEMORY_BASIC_INFORMATION mbi;
	int Count;
	while ( VirtualQueryEx( hProcess, reinterpret_cast< LPCVOID >( StartAddress ), &mbi, sizeof( mbi ) ) != 0 ) {
		Count = 0;
		auto BlockSize = mbi.RegionSize;

		while ( BlockSize >= BLOCKMAXSIZE ) {
			if ( ResultArray.size( ) >= SearchNum )
				goto END;

			SearchMemoryBlock( MemoryBuffer, NextArray, SignatureArray, StartAddress + ( BLOCKMAXSIZE * Count ), BLOCKMAXSIZE, ResultArray );

			BlockSize -= BLOCKMAXSIZE;
			Count++;
		}

		SearchMemoryBlock( MemoryBuffer, NextArray, SignatureArray, StartAddress + ( BLOCKMAXSIZE * Count ), BlockSize, ResultArray );

		StartAddress += mbi.RegionSize;

		if ( ResultArray.size( ) >= SearchNum || EndAddress != 0 && StartAddress > EndAddress )
			break;
	}

END:

	delete[ ] MemoryBuffer;
	return ResultArray;
}