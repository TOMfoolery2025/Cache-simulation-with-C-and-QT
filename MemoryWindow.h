#ifndef MEMORYWINDOW_H
#define MEMORYWINDOW_H

#include <QDialog>
#include <QGraphicsScene>

namespace Ui {
class MemoryWindow;
}

class MemoryWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MemoryWindow(int blockSize, QWidget *parent = nullptr);
    ~MemoryWindow();

private:
    Ui::MemoryWindow *ui;
    QGraphicsScene *scene;

    void drawMemory(int blockSize);
};

#endif // MEMORYWINDOW_H
