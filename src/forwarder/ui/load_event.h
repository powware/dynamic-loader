#ifndef __LOAD_EVENT_H__
#define __LOAD_EVENT_H__

#include <QEvent>

#include <Windows.h>

const QEvent::Type LoadEventType = static_cast<QEvent::Type>(QEvent::registerEventType());

class LoadEvent : public QEvent
{
public:
    LoadEvent(QIcon process_icon, QString process, DWORD process_id, std::wstring dll, std::optional<HMODULE> module) : QEvent(LoadEventType), process_icon(std::move(process_icon)), process(std::move(process)), process_id(process_id), dll(std::move(dll)), module(module) {}
    QIcon process_icon;
    QString process;
    DWORD process_id;
    std::wstring dll;
    std::optional<HMODULE> module;
};

#endif // __LOAD_EVENT_H__