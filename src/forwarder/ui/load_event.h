#ifndef __LOAD_EVENT_H__
#define __LOAD_EVENT_H__

#include <QEvent>

#include <Windows.h>

const QEvent::Type LoadEventType = static_cast<QEvent::Type>(QEvent::registerEventType());

class LoadEvent : public QEvent
{
public:
    LoadEvent(std::optional<HMODULE> module) : QEvent(LoadEventType), module_(module) {}
    std::optional<HMODULE> module_;
};

#endif // __LOAD_EVENT_H__