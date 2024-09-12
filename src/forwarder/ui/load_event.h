#ifndef __LOAD_EVENT_H__
#define __LOAD_EVENT_H__

#include <QEvent>

#include <Windows.h>

const QEvent::Type LoadEventType = static_cast<QEvent::Type>(QEvent::registerEventType());

class LoadEvent : public QEvent
{
public:
    LoadEvent(std::wstring process, DWORD process_id, std::wstring dll, std::optional<HMODULE> module) : QEvent(LoadEventType), process(process), process_id(process_id), dll(dll), module(module) {}
    std::wstring process;
    DWORD process_id;
    std::wstring dll;
    std::optional<HMODULE> module;
};

#endif // __LOAD_EVENT_H__