#ifndef BRUSH_FILL_H
#define BRUSH_FILL_H

#include "Brush.h"
#include <QRgb>
#include <QList>
#include <QPoint>

class QUndoStack;

class Brush_Fill : public Brush {
    QUndoStack* undo;
public:
    Brush_Fill(MainWidget* w, QUndoStack* u) : Brush(w), undo(u) {}
    bool onMouseRelease(QMouseEvent* event, int x, int y, bool insideWidget);
private:
    QList<QPoint> pixels;
};

#endif // BRUSH_FILL_H
