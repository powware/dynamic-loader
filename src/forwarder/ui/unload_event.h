#ifndef __UNLOAD_EVENT_H__
#define __UNLOAD_EVENT_H__

#include <QEvent>

#include <pfw.h>

const QEvent::Type UnloadEventType = static_cast<QEvent::Type>(QEvent::registerEventType());

class UnloadEvent : public QEvent
{
public:
    UnloadEvent(QString process_id, QString module, bool success) : QEvent(UnloadEventType), process_id(process_id), module(module), success(success) {}
    QString process_id;
    QString module;
    bool success;
};

#endif // __UNLOAD_EVENT_H__