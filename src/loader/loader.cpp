#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <pfw.h>

#include "ipc/pipe.h"
#include "ipc/serialization.h"

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

std::optional<HMODULE> LoadModule(HANDLE process, std::wstring dll)
{

	const auto kernel_module = pfw::GetRemoteModuleHandle(process, L"Kernel32.dll");
	if (!kernel_module)
	{
		return std::nullopt;
	}

	const auto load_library = pfw::GetRemoteProcAddress(process, *kernel_module, "LoadLibraryW");
	if (!load_library)
	{
		return std::nullopt;
	}

	const auto dll_size = (dll.size() + 1) * sizeof(std::wstring::traits_type::char_type);

	auto load_library_arg = pfw::RemoteVirtualMemory::Create(process, dll_size);
	if (!load_library_arg)
	{
		return std::nullopt;
	}

	if (!pfw::SetRemoteMemory(*load_library_arg, dll.c_str(), dll_size)) // c_str guarantees the trailing '\0'
	{
		return std::nullopt;
	}

	auto loader = RemoteThread::Create(process, *load_library, load_library_arg->get());
	if (!loader)
	{
		return std::nullopt;
	}

	loader->Join();

	const auto exit_code = loader->GetExitCode();
	if (!exit_code)
	{
		return std::nullopt;
	}

	return pfw::GetRemoteModuleHandle(process, std::filesystem::path(dll).filename().wstring());
}

bool UnloadModule(HANDLE process, HMODULE module)
{

	auto kernel_module = pfw::GetRemoteModuleHandle(process, L"Kernel32.dll");
	if (!kernel_module)
	{
		return false;
	}

	auto free_library = pfw::GetRemoteProcAddress(process, *kernel_module, "FreeLibrary");
	if (!free_library)
	{
		return false;
	}

	auto free_arg = pfw::RemoteVirtualMemory::Create(process, sizeof(HMODULE));
	if (!free_arg)
	{
		return false;
	}

	if (!pfw::SetRemoteMemory(*free_arg, module))
	{
		return false;
	}

	auto unloader = RemoteThread::Create(process, *free_library, free_arg->get());
	if (!unloader)
	{
		return false;
	}

	unloader->Join();

	const auto exit_code = unloader->GetExitCode();
	if (!exit_code)
	{
		return false;
	}

	return true;
}

class Handler
{
public:
	static std::unique_ptr<Handler> Create()
	{
		auto read_handle = pfw::Handle::Create(GetStdHandle(STD_INPUT_HANDLE));
		if (!read_handle)
		{
			return nullptr;
		}

		auto write_handle = pfw::Handle::Create(GetStdHandle(STD_OUTPUT_HANDLE));
		if (!write_handle)
		{
			return nullptr;
		}

		return std::unique_ptr<Handler>(new Handler(std::move(*read_handle), std::move(*write_handle)));
	}

	void WriteThread(std::unique_ptr<WritePipe> pipe, std::stop_token stoken)
	{
		while (!stoken.stop_requested())
		{
			std::unique_lock lock(mutex_);
			cv_.wait(lock, stoken, [this, stoken]
					 { return queue_.size(); });
			if (stoken.stop_requested())
			{
				return;
			}

			do
			{
				pipe->Write(queue_.front());
				queue_.pop();
			} while (queue_.size());
		}
	}

	void Read()
	{
		while (auto buffer = read_pipe_->Read())
		{
			Deserializer deserializer(*buffer); // ByteInStream istream(std::move(*buffer));
			RemoteProcedure rpc;
			deserializer.deserialize(rpc);
			uint8_t seq;
			deserializer.deserialize(seq);

			switch (rpc)
			{
			case RemoteProcedure::Load:
			{
				DWORD process_id;
				deserializer.deserialize(process_id);
				std::wstring dll;
				deserializer.deserialize(dll);
				auto module = Load(process_id, std::move(dll));
				Write(Serializer().serialize(rpc).serialize(seq).serialize(module).buffer());
			}
			break;
			case RemoteProcedure::Unload:
			{
				DWORD process_id;
				deserializer.deserialize(process_id);
				HMODULE module;
				deserializer.deserialize(module);
				auto success = Unload(process_id, module);
				Write(Serializer().serialize(rpc).serialize(seq).serialize(success).buffer());
			}
			break;
			case RemoteProcedure::Close:
			{
			}
			break;
			default:
			{
			}
			break;
			}
		}
	}

	std::optional<HMODULE> Load(DWORD process_id, std::wstring dll)
	{

		auto process = pfw::OpenProcess(process_id);
		if (!process)
		{
			return std::nullopt;
		}

		auto module = LoadModule(*process, dll);
		processes_.emplace(std::make_pair(process_id, std::move(*process)));
		return module;
	}

	bool Unload(DWORD process_id, HMODULE module)
	{

		auto process = processes_.find(process_id);
		if (process == processes_.end())
		{
			return false;
		}

		return UnloadModule(process->second, module);
	}

	template <typename Buffer>
	void Write(Buffer &&arg)
	{
		std::scoped_lock lock(mutex_);
		queue_.push(std::forward<Buffer>(arg));
		cv_.notify_one();
	}

	Handler(const Handler &) = delete;
	auto operator=(const Handler &) = delete;

private:
	std::unordered_map<DWORD, pfw::Handle> processes_;
	std::unique_ptr<ReadPipe> read_pipe_;
	std::mutex mutex_;
	std::condition_variable_any cv_;
	std::queue<std::vector<std::byte>> queue_;
	std::jthread thread_; // destroys before the variables used inside of it, also destroys write_pipe before read_pipe
	Handler(pfw::Handle &&read_handle, pfw::Handle &&write_handle) : read_pipe_(std::make_unique<ReadPipe>(std::forward<pfw::Handle>(read_handle)))
	{
		thread_ = std::jthread(std::bind_front(&Handler::WriteThread, this, std::make_unique<WritePipe>(std::forward<pfw::Handle>(write_handle))));
	}
};

int wmain()
{
	if (!pfw::SetDebugPrivileges())
	{
		// std::cout << "SetDebugPrivileges failed. You may need to restart with admin privileges.\n";
	}

	auto handler = Handler::Create();
	if (!handler)
	{
		return EXIT_FAILURE;
	}
	handler->Read();

	return EXIT_SUCCESS;
}