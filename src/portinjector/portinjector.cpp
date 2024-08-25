#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include "pfw.h"

// class FileHandle
// {
// public:
// 	FileHandle() : handle_(INVALID_HANDLE_VALUE) {}

// 	FileHandle(std::string_view path, DWORD desired_access, DWORD share_mode, DWORD creation_dispostion)
// 	{
// 		this->handle_ = CreateFileA(path.data(), desired_access, share_mode, nullptr, creation_dispostion, 0, nullptr);
// 		if (this->handle_ == INVALID_HANDLE_VALUE)
// 			throw "derive this";
// 	}

// 	FileHandle(const FileHandle &file_handle) = delete; // requires reference counting

// 	FileHandle(FileHandle &&file_handle) noexcept
// 	{
// 		this->handle_ = file_handle.handle_;
// 		file_handle.handle_ = INVALID_HANDLE_VALUE;
// 	}

// 	~FileHandle()
// 	{
// 		if (this->handle_ != INVALID_HANDLE_VALUE)
// 			CloseHandle(this->handle_);
// 	}

// 	FileHandle &operator=(const FileHandle &file_handle) = delete;

// 	FileHandle &operator=(FileHandle &&file_handle) noexcept
// 	{
// 		this->handle_ = file_handle.handle_;
// 		file_handle.handle_ = INVALID_HANDLE_VALUE;
// 		return *this;
// 	}

// 	template <typename T>
// 	operator T() const
// 	{
// 		if (this->handle_ == INVALID_HANDLE_VALUE)
// 			throw "uninitalized";
// 		return this->handle_;
// 	}

// private:
// 	HANDLE handle_;
// };

// class File
// {
// public:
// 	std::string name_;
// 	std::string path_;
// 	std::size_t size_{};

// 	File(std::string name, std::string path) : name_(name), path_(path)
// 	{
// 	}

// 	File(std::string path) : path_(path), name_(std::filesystem::exe))
// 	{
// 	}

// protected:
// 	File() = default;

// 	DWORD SetFileSize(const FileHandle &file_handle)
// 	{
// 		DWORD file_size_high;
// 		DWORD file_size_low = ::GetFileSize(file_handle, &file_size_high);
// 		if (file_size_high)
// 			throw "not yet implemented";
// 		this->size_ = file_size_low;
// 	}
// };

// class DllFile : public File
// {
// public:
// 	std::string base_name_;

// 	DllFile(std::string path) : File(std::forward<std::string>(path))
// 	{
// 		FileHandle file_handle(this->path_, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
// 		this->SetFileSize(file_handle);
// 	}
// };

// class Process
// {
// public:
// 	Process(std::string_view process_name) : process_id_(pfw::GetProcessId(process_name)) {}

// 	Process(DWORD process_id) : process_id_(process_id) {}

// 	pfw::ProcessHandle GetProcessHandle()
// 	{
// 		return pfw::ProcessHandle(process_id_);
// 	}

// private:
// 	DWORD process_id_;
// };

// class VirtualMemory
// {
// public:
// 	VirtualMemory(HANDLE process_handle, void *target_address, std::size_t size, DWORD allocation_type, DWORD protection, bool raii = true) : process_handle_(process_handle),
// 																																			  handle_(VirtualAllocEx(process_handle, target_address, size, allocation_type, protection)), size_(size), remote_(true), raii_(raii)
// 	{
// 		if (this->handle_ == nullptr)
// 			throw std::bad_alloc();
// 	};

// 	VirtualMemory(void *target_address, std::size_t size, DWORD allocation_type, DWORD protection) : process_handle_(GetCurrentProcess()),
// 																									 handle_(VirtualAlloc(target_address, size, allocation_type, protection)), size_(size), remote_(false), raii_(true)
// 	{
// 		if (this->handle_ == nullptr)
// 			throw std::bad_alloc();
// 	};

// 	~VirtualMemory()
// 	{
// 		if (this->handle_ && this->raii_)
// 		{
// 			if (this->remote_)
// 				VirtualFreeEx(process_handle_, handle_, 0, MEM_RELEASE);
// 			else
// 				VirtualFree(handle_, 0, MEM_RELEASE);
// 		}
// 	}

// 	template <typename T>
// 	operator T() const
// 	{
// 		return handle_;
// 	}

// 	void DisableRAII()
// 	{
// 		this->raii_ = false;
// 	}

// private:
// 	HANDLE handle_;
// 	HANDLE process_handle_;
// 	const std::size_t size_;
// 	const bool remote_;
// 	bool raii_;
// };

// class Module
// {
// public:
// 	virtual void Detach() = 0;

// protected:
// 	Module(Process process, std::string dll_path) : process_(process), dll_path_(dll_path) {};
// 	Process process_;
// 	HANDLE handle_ = nullptr;
// 	std::string dll_path_;
// 	std::size_t size_ = 0;
// };

// class ManuallyMappedModule : public Module
// {
// public:
// 	ManuallyMappedModule(Process process, std::string dll_path) : Module(process, dll_path)
// 	{
// 		pfw::ProcessHandle process_handle = process_.GetProcessHandle();
// 		std::intptr_t loader_delta = reinterpret_cast<char *>(LoaderEnd) - reinterpret_cast<char *>(Loader);
// 		if (loader_delta <= 0)
// 			throw "Recompile.";
// 		std::size_t loader_size = loader_delta;

// 		FileHandle file_handle(this->dll_path_, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);

// 		DWORD file_size_high;
// 		DWORD file_size_low = ::GetFileSize(file_handle, &file_size_high);
// 		if (file_size_high)
// 			throw "not yet implemented.";

// 		VirtualMemory file_memory(nullptr, file_size_low, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
// 		if (!ReadFile(file_handle, file_memory, file_size_low, NULL, NULL))
// 			throw "Couldnt read file.";

// 		std::size_t module_size = pfw::pefile::GetOptionalHeader(file_memory)->SizeOfImage;
// 		pfw::RemoteVirtualMemory module_memory(process_handle, nullptr, module_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

// 		pfw::SetRemoteMemory(process_handle, module_memory, file_memory, pfw::pefile::GetOptionalHeader(file_memory)->SizeOfHeaders);

// 		IMAGE_SECTION_HEADER *sectionHeaderList = pfw::pefile::GetSectionHeaderList(file_memory);
// 		for (std::size_t i = 0; i < pfw::pefile::GetOptionalHeader(file_memory)->NumberOfRvaAndSizes && sectionHeaderList[i].SizeOfRawData; i++)
// 		{
// 			pfw::SetRemoteMemory(process_handle, static_cast<char *>(module_memory) + sectionHeaderList[i].VirtualAddress,
// 								 static_cast<char *>(file_memory) + sectionHeaderList[i].PointerToRawData, sectionHeaderList[i].SizeOfRawData);
// 		}

// 		VirtualMemory loader_memory(process_handle, nullptr, loader_size + sizeof(LoaderParameters), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
// 		pfw::SetRemoteMemory(process_handle, loader_memory, Loader, loader_size);

// 		LoaderParameters parameters;
// 		parameters.module_base = module_memory;
// 		parameters.desired_module_base = reinterpret_cast<void *>(pfw::pefile::GetOptionalHeader(file_memory)->ImageBase);
// 		parameters.base_reloction_table_offset = pfw::pefile::GetDataDirectory(file_memory, IMAGE_DIRECTORY_ENTRY_BASERELOC).VirtualAddress;
// 		parameters.import_directory_table_offset = pfw::pefile::GetDataDirectory(file_memory, IMAGE_DIRECTORY_ENTRY_IMPORT).VirtualAddress;
// 		HMODULE kernel_module = pfw::GetRemoteModuleHandle(process_handle, "Kernel32.dll");
// 		parameters.load_library = pfw::GetRemoteProcAddress<LoadLibrary_>(process_handle, kernel_module, "LoadLibraryA");
// 		parameters.get_proc_address = pfw::GetRemoteProcAddress<GetProcAddress_>(process_handle, kernel_module, "GetProcAddress");
// 		this->entry_point_offset = pfw::pefile::GetOptionalHeader(file_memory)->AddressOfEntryPoint;
// 		parameters.entry_point_offset = this->entry_point_offset;
// 		pfw::SetRemoteMemory(process_handle, static_cast<char *>(loader_memory) + loader_size, parameters);

// 		pfw::RemoteThread loader_thread(process_handle, loader_memory, static_cast<char *>(loader_memory) + loader_size);
// 		loader_thread.Join();
// 		if (loader_thread.GetExitCode())
// 			throw "failed";
// 		this->handle_ = module_memory;
// 	}

// 	void Detach() override
// 	{
// 		pfw::ProcessHandle process_handle = process_.GetProcessHandle();
// 		intptr_t unloader_delta = reinterpret_cast<char *>(UnloadEnd) - reinterpret_cast<char *>(Unload);
// 		if (unloader_delta <= 0)
// 			throw "Recompile.";
// 		size_t unloader_size = unloader_delta;

// 		VirtualMemory unloader_memory(process_handle, nullptr, unloader_size + sizeof(UnloaderParameters), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
// 		pfw::SetRemoteMemory(process_handle, unloader_memory, Unload, unloader_size);

// 		UnloaderParameters parameters;
// 		parameters.module_base = this->handle_;
// 		parameters.entry_point_offset = this->entry_point_offset;
// 		pfw::SetRemoteMemory(process_handle, static_cast<char *>(unloader_memory) + unloader_size, parameters);

// 		HANDLE thread_handle = CreateRemoteThread(process_handle, NULL, 0, static_cast<LPTHREAD_START_ROUTINE>(unloader_memory), static_cast<char *>(unloader_memory) + unloader_size, 0, NULL);
// 		WaitForSingleObject(thread_handle, INFINITE);
// 		DWORD exit_code_thread;
// 		GetExitCodeThread(thread_handle, &exit_code_thread);
// 		if (exit_code_thread)
// 			throw "failed";
// 		this->handle_ = nullptr;
// 	}

// private:
// 	DWORD entry_point_offset = 0;

// 	using LoadLibrary_ = HMODULE(WINAPI *)(LPCSTR);
// 	using GetProcAddress_ = FARPROC(WINAPI *)(HMODULE, LPCSTR);
// 	using DllMain_ = BOOL(WINAPI *)(HMODULE, DWORD, LPVOID);

// 	using LoaderParameters = struct
// 	{
// 		void *module_base;
// 		void *desired_module_base;
// 		DWORD entry_point_offset;
// 		DWORD base_reloction_table_offset;
// 		DWORD import_directory_table_offset;
// 		LoadLibrary_ load_library;
// 		GetProcAddress_ get_proc_address;
// 	};

// 	static DWORD WINAPI Loader(void *parameters)
// 	{
// 		LoaderParameters *loader_parameters = static_cast<LoaderParameters *>(parameters);
// 		intptr_t module_base_delta = static_cast<char *>(loader_parameters->module_base) - static_cast<char *>(loader_parameters->desired_module_base);

// 		if (module_base_delta)
// 		{
// 			IMAGE_BASE_RELOCATION *base_relocation = reinterpret_cast<IMAGE_BASE_RELOCATION *>(static_cast<char *>(loader_parameters->module_base) + loader_parameters->base_reloction_table_offset);
// 			while (base_relocation->VirtualAddress)
// 			{
// 				int count = (base_relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
// 				WORD *offset_block = reinterpret_cast<WORD *>(base_relocation + 1);
// 				for (int i = 0; i < count - 1; i++)
// 				{
// 					if (offset_block[i])
// 					{
// 						*reinterpret_cast<uintptr_t *>(static_cast<char *>(loader_parameters->module_base) + base_relocation->VirtualAddress + (offset_block[i] & 0xFFF)) += module_base_delta;
// 					}
// 				}
// 				base_relocation = reinterpret_cast<IMAGE_BASE_RELOCATION *>(reinterpret_cast<char *>(base_relocation) + base_relocation->SizeOfBlock);
// 			}
// 		}

// 		IMAGE_IMPORT_DESCRIPTOR *import_directory = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(static_cast<char *>(loader_parameters->module_base) + loader_parameters->import_directory_table_offset);
// 		while (import_directory->Characteristics)
// 		{
// 			HMODULE module_handle = loader_parameters->load_library(static_cast<char *>(loader_parameters->module_base) + import_directory->Name);
// 			if (!module_handle || module_handle == INVALID_HANDLE_VALUE)
// 				return 1;

// 			IMAGE_THUNK_DATA *import_lookup_table = reinterpret_cast<IMAGE_THUNK_DATA *>(static_cast<char *>(loader_parameters->module_base) + import_directory->OriginalFirstThunk);
// 			IMAGE_THUNK_DATA *import_address_table = reinterpret_cast<IMAGE_THUNK_DATA *>(static_cast<char *>(loader_parameters->module_base) + import_directory->FirstThunk);
// 			while (import_lookup_table->u1.AddressOfData)
// 			{
// 				if (import_lookup_table->u1.Ordinal & IMAGE_ORDINAL_FLAG)
// 				{
// 					char *ordinal = reinterpret_cast<char *>(import_lookup_table->u1.Ordinal & 0xFFFF);
// 					uintptr_t function_address = reinterpret_cast<uintptr_t>(loader_parameters->get_proc_address(module_handle, ordinal));
// 					if (!function_address)
// 						return 1;
// 					import_address_table->u1.Function = function_address;
// 				}
// 				else
// 				{
// 					IMAGE_IMPORT_BY_NAME *importName = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(static_cast<char *>(loader_parameters->module_base) + import_lookup_table->u1.AddressOfData);
// 					uintptr_t function_address = reinterpret_cast<uintptr_t>(loader_parameters->get_proc_address(module_handle, importName->Name));
// 					if (!function_address)
// 						return 1;
// 					import_address_table->u1.Function = function_address;
// 				}
// 				import_lookup_table++;
// 				import_address_table++;
// 			}
// 			import_directory++;
// 		}
// 		DllMain_ dll_main = reinterpret_cast<DllMain_>(static_cast<char *>(loader_parameters->module_base) + loader_parameters->entry_point_offset);
// 		return dll_main(static_cast<HINSTANCE>(loader_parameters->module_base), DLL_PROCESS_ATTACH, NULL) ? 0 : 1;
// 	}

// 	static DWORD WINAPI LoaderEnd()
// 	{
// 		return 0;
// 	}

// 	using UnloaderParameters = struct
// 	{
// 		void *module_base;
// 		DWORD entry_point_offset;
// 	};

// 	static DWORD WINAPI Unload(void *parameters)
// 	{
// 		UnloaderParameters *unloader_parameters = reinterpret_cast<UnloaderParameters *>(parameters);
// 		DllMain_ dll_main = reinterpret_cast<DllMain_>(static_cast<char *>(unloader_parameters->module_base) + unloader_parameters->entry_point_offset);
// 		return dll_main(static_cast<HINSTANCE>(unloader_parameters->module_base), DLL_PROCESS_DETACH, NULL) ? 0 : 1;
// 	}

// 	static DWORD WINAPI UnloadEnd()
// 	{
// 		return 0;
// 	}
// };

// class LibraryModule : public Module
// {
// public:
// 	LibraryModule(Process process, std::string dll_path) : Module(process, dll_path)
// 	{
// 		pfw::ProcessHandle process_handle = process_.GetProcessHandle();
// 		VirtualMemory loader_memory(process_handle, nullptr, this->dll_path_.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
// 		HMODULE kernel_module = pfw::GetRemoteModuleHandle(process_handle, "Kernel32.dll");
// 		void *load_library = pfw::GetRemoteProcAddress(process_handle, kernel_module, "LoadLibraryA");
// 		pfw::stringutils::SetRemoteString(process_handle, loader_memory, this->dll_path_);
// 		pfw::RemoteThread loader_thread(process_handle, load_library, loader_memory);
// 		loader_thread.Join();
// 		// this->handle_ = loader_thread.GetExitCode();
// 	}

// 	void Detach() override
// 	{
// 		pfw::ProcessHandle process_handle = process_.GetProcessHandle();
// 		VirtualMemory module_handle_memory(process_handle, nullptr, sizeof(HMODULE), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
// 		pfw::SetRemoteMemory(process_handle, module_handle_memory, this->handle_);
// 		HMODULE kernel_module = pfw::GetRemoteModuleHandle(process_handle, "Kernel32.dll");
// 		void *free_library = pfw::GetRemoteProcAddress(process_handle, kernel_module, "FreeLibrary");
// 		pfw::RemoteThread loader_thread(process_handle, free_library, module_handle_memory);
// 		loader_thread.Join();
// 	}
// };

// int ppmain()
// {
// 	std::vector<std::unique_ptr<Module>> modules;
// 	while (true)
// 	{
// 		std::cin.get();
// 		try
// 		{
// 			Process ac_client("ac_client.exe");
// 			ManuallyMappedModule assaultcube(ac_client, "assaultcube.dll");
// 			// modules.push_back(std::make_unique<ManuallyMappedModule>(ac_client, "assaultcube.dll"));
// 			std::cout << "module attached" << std::endl;
// 			std::cin.get();
// 			try
// 			{
// 				assaultcube.Detach();
// 				std::cout << "module detached" << std::endl;
// 			}
// 			catch (...)
// 			{
// 				std::cout << "failed to detach module" << std::endl;
// 			}
// 		}
// 		catch (...)
// 		{
// 			std::cout << "failed to attach module" << std::endl;
// 		}
// 	}
// 	return 0;
// }

int main()
{
	if (!pfw::SetDebugPrivileges())
	{
		std::cout << "SetDebugPrivileges failed. You may need to restart with admin privileges.\n";
	}

	// Process ac_client("ac_client.exe");
	// LibraryModule assaultcube(ac_client, "C:\\Users\\powware\\repos\\assaultcube\\build\\Release\\assaultcube.dll");
	return EXIT_SUCCESS;
}