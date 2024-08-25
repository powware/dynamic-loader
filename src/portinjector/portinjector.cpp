#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

#include <pfw.h>

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

// };

class VirtualMemory
{
public:
	static std::optional<VirtualMemory> Allocate(HANDLE process_handle, std::size_t size)
	{
		void *memory = VirtualAllocEx(process_handle, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		return memory ? std::make_optional<VirtualMemory>(VirtualMemory(process_handle, memory)) : std::nullopt;
	}

	static std::optional<VirtualMemory> Allocate(std::size_t size)
	{
		void *memory = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		return memory ? std::make_optional<VirtualMemory>(memory) : std::nullopt;
	}

	VirtualMemory(const VirtualMemory &) = delete;
	VirtualMemory(VirtualMemory &&other) noexcept : process_handle_(other.process_handle_), memory_(other.memory_)
	{
		other.memory_ = nullptr;
	}

	VirtualMemory &operator=(const VirtualMemory) = delete;
	VirtualMemory &operator=(VirtualMemory &) = delete;

	~VirtualMemory()
	{
		if (memory_)
		{
			if (process_handle_)
			{
				VirtualFreeEx(*process_handle_, memory_, 0, MEM_RELEASE);
			}
			else
			{
				VirtualFree(memory_, 0, MEM_RELEASE);
			}
		}
	}
	void *operator*()
	{
		return memory_;
	}

	void *get()
	{
		return memory_;
	}

private:
	VirtualMemory(HANDLE process_handle, void *memory) : process_handle_(process_handle), memory_(memory) {};
	VirtualMemory(void *memory) : process_handle_(std::nullopt), memory_(memory) {};

	std::optional<HANDLE> process_handle_;
	void *memory_;
};

class RemoteThread
{
public:
	static std::optional<RemoteThread> Create(HANDLE process_handle, void *start_routine, void *arguments)
	{
		HANDLE handle = CreateRemoteThread(process_handle, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(start_routine), arguments, 0, nullptr);
		return handle ? std::make_optional<RemoteThread>(handle) : std::nullopt;
	}

	bool Join(DWORD milliseconds = INFINITE)
	{
		DWORD result = WaitForSingleObject(handle_, milliseconds);
		if (result == WAIT_OBJECT_0)
		{
			return true;
		}

		return false;
	}

	std::optional<DWORD> GetExitCode()
	{
		DWORD exit_code;
		if (!GetExitCodeThread(handle_, &exit_code))
		{
			return std::nullopt;
		}

		return exit_code;
	}

	HANDLE operator*()
	{
		return handle_;
	}

	HANDLE get()
	{
		return handle_;
	}

private:
	RemoteThread(HANDLE handle) : handle_(handle) {}

	HANDLE handle_;
};

bool LoadModule(HANDLE process_handle, std::wstring dll_path)
{

	auto kernel_module = pfw::GetRemoteModuleHandle(process_handle, L"Kernel32.dll");
	if (!kernel_module)
	{
		return false;
	}

	auto load_library = pfw::GetRemoteProcAddress(process_handle, *kernel_module, "LoadLibraryW");
	if (!load_library)
	{
		return false;
	}

	const auto dll_path_size_in_bytes = (dll_path.size() + 1) * sizeof(std::wstring::traits_type::char_type);

	auto load_library_arg = VirtualMemory::Allocate(process_handle, dll_path_size_in_bytes);
	if (!load_library_arg)
	{
		return false;
	}

	if (!pfw::SetRemoteMemory(process_handle, **load_library_arg, dll_path.c_str(), dll_path_size_in_bytes))
	{
		return false;
	}

	auto loader_thread = RemoteThread::Create(process_handle, *load_library, **load_library_arg);
	if (!loader_thread)
	{
		return false;
	}
	loader_thread->Join();

	auto exit_code = loader_thread->GetExitCode();
	if (!exit_code)
	{
		return false;
	}

	return true;
}

// void UnloadModule(HANDLE process_handle)
// {
// 	pfw::ProcessHandle process_handle = process_.GetProcessHandle();
// 	VirtualMemory module_handle_memory(process_handle, nullptr, sizeof(HMODULE), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
// 	pfw::SetRemoteMemory(process_handle, module_handle_memory, this->handle_);
// 	HMODULE kernel_module = pfw::GetRemoteModuleHandle(process_handle, "Kernel32.dll");
// 	void *free_library = pfw::GetRemoteProcAddress(process_handle, kernel_module, "FreeLibrary");
// 	pfw::RemoteThread loader_thread(process_handle, free_library, module_handle_memory);
// 	loader_thread.Join();
// }

int main(int argc, char *argv[])
{
	if (!pfw::SetDebugPrivileges())
	{
		std::cout << "SetDebugPrivileges failed. You may need to restart with admin privileges.\n";
	}

	std::optional<DWORD> process_id{};
	std::optional<std::wstring> dll_path{};
	std::optional<bool> load{};
	for (int i = 0; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "--pid" && i + 1 < argc)
		{
			process_id = std::stoul(argv[++i]);
		}
		else if (arg == "--dll" && i + 1 < argc)
		{
			std::string arg2(argv[++i]);
			dll_path = std::wstring();
			dll_path->resize(arg2.size());
			std::mbstowcs(dll_path->data(), arg2.c_str(), arg2.size());
		}
		else if (arg == "--load")
		{
			load = true;
		}
		else if (arg == "--unload")
		{
			load = false;
		}
	}

	if (!process_id || !dll_path || !load)
	{
		return EXIT_FAILURE;
	}

	auto process_handle = pfw::OpenProcess(*process_id);
	if (!process_handle)
	{
		return EXIT_FAILURE;
	}

	if (*load)
	{
		return LoadModule((*process_handle).get(), *dll_path) ? EXIT_SUCCESS : EXIT_FAILURE;
	}
	else
	{
		// UnloadModule((*process_handle).get(), *dll_path);
	}

	// Process ac_client("ac_client.exe");
	// LibraryModule assaultcube(ac_client, "C:\\Users\\powware\\repos\\assaultcube\\build\\Release\\assaultcube.dll");
	return EXIT_SUCCESS;
}