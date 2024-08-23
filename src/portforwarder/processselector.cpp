#include "processselector.h"

#include <QDebug>

ProcessSelector::ProcessSelector(QWidget *parent) : QComboBox(parent) {}

void ProcessSelector::showPopup()
{
    emit popup();

    QComboBox::showPopup();
}