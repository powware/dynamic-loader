#ifndef __UNLOAD_EVENT_H__
#define __UNLOAD_EVENT_H__

#include <QEvent>

#include <Windows.h>

const QEvent::Type UnloadEventType = static_cast<QEvent::Type>(QEvent::registerEventType());

class UnloadEvent : public QEvent
{
public:
    UnloadEvent(bool success) : QEvent(UnloadEventType), success_(success) {}
    bool success_;
};

#endif // __UNLOAD_EVENT_H__