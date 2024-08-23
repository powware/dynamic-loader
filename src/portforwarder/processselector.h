#ifndef __PROCESSSELECTOR_H__
#define __PROCESSSELECTOR_H__

#include <QComboBox>
#include <QShowEvent>

class ProcessSelector : public QComboBox
{
    Q_OBJECT
public:
    explicit ProcessSelector(QWidget *parent = nullptr);

signals:
    void popup();

protected:
    void showPopup() override;
};
#endif // __PROCESSSELECTOR_H__