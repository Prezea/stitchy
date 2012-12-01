#include <QMouseEvent>
#include <QWheelEvent>

#include "cell.h"
#include "document.h"
#include "editor.h"
#include "editoractions.h"
#include "globalstate.h"
#include "selection.h"
#include "sparsemap.h"

#include "canvas.h"

#define MAGNIFICATION_RATE 0.2

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent)
{
  selection_ = NULL;
  drawmap_ = NULL;

  selecting_ = false;
  moving_ = false;
  dragging_ = false;
  drawing_ = false;
  erasing_ = false;
  rectangle_ = false;

  cursor_ = QPoint(-1, -1);
  subcursor_ = Subarea_TopLeft;
}

Canvas::~Canvas()
{

}

bool Canvas::mapToGrid(const QPoint &pos, QPoint &out)
{
  Document *d = GlobalState::self()->activeDocument();
  if (!d)
    return false;

  const QSize dim = d->size();

  QPointF coord = mapToScene(pos);
  QPoint cint(coord.x() / 10, coord.y() / 10);

  if (cint.x() < 0 || cint.x() >= dim.width() ||
      cint.y() < 0 || cint.y() >= dim.height())
    return false;

  out = cint;
  coord.setX(coord.x() - (cint.x() * 10));
  coord.setY(coord.y() - (cint.y() * 10));

  return true;
}

bool Canvas::mapToGrid(const QPoint &pos, QPoint &out, Subarea &subareaOut)
{
  Document *d = GlobalState::self()->activeDocument();
  if (!d)
    return false;

  const QSize dim = d->size();

  QPointF coord = mapToScene(pos);
  QPoint cint(coord.x() / 10, coord.y() / 10);

  if (cint.x() < 0 || cint.x() >= dim.width() ||
      cint.y() < 0 || cint.y() >= dim.height())
    return false;

  out = cint;
  coord.setX(coord.x() - (cint.x() * 10));
  coord.setY(coord.y() - (cint.y() * 10));

  if (coord.x() > 5.0f) {
    if (coord.y() > 5.0f)
      subareaOut = Subarea_BottomRight;
    else
      subareaOut = Subarea_TopRight;
  } else {
    if (coord.y() > 5.0f)
      subareaOut = Subarea_BottomLeft;
    else
      subareaOut = Subarea_TopLeft;
  }

  return true;
}

void Canvas::zoomIn()
{
  scale(1.0 + MAGNIFICATION_RATE, 1.0 + MAGNIFICATION_RATE);
}

void Canvas::zoomOut()
{
  scale(1.0 - MAGNIFICATION_RATE, 1.0 - MAGNIFICATION_RATE);
}

void Canvas::zoomReset()
{
  resetMatrix();
}

void Canvas::toggleGrid(bool enabled)
{

}

void Canvas::setCenter(const QPointF& centerPoint)
{
  //Get the rectangle of the visible area in scene coords
  QRectF visibleArea = mapToScene(rect()).boundingRect();
  
  //Get the scene area
  QRectF sceneBounds = sceneRect();
  
  double boundX = sceneBounds.x() + visibleArea.width() / 2.0;
  double boundY = sceneBounds.y() + visibleArea.height() / 2.0;
  double boundWidth = sceneBounds.width() - 2.0 * boundX;
  double boundHeight = sceneBounds.height() - 2.0 * boundY;
  
  //The max boundary that the centerPoint can be to
  QRectF bounds(boundX, boundY, boundWidth, boundHeight);

  if(bounds.contains(centerPoint)) {
    //We are within the bounds
    center_ = centerPoint;
  } else {
    //We need to clamp or use the center of the screen
    if(visibleArea.contains(sceneBounds)) {
      //Use the center of scene ie. we can see the whole scene
      center_ = sceneBounds.center();
    } else {
      center_ = centerPoint;
      
      //We need to clamp the center. The centerPoint is too large
      if(centerPoint.x() > bounds.x() + bounds.width()) {
        center_.setX(bounds.x() + bounds.width());
      } else if(centerPoint.x() < bounds.x()) {
        center_.setX(bounds.x());
      }
      
      if(centerPoint.y() > bounds.y() + bounds.height()) {
        center_.setY(bounds.y() + bounds.height());
      } else if(centerPoint.y() < bounds.y()) {
        center_.setY(bounds.y());
      }
      
    }
  }
  
  //Update the scrollbars
  centerOn(center_);
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
  if (event->button() & Qt::LeftButton) {
    ToolMode mode = GlobalState::self()->toolMode();

    if (mode == ToolMode_Select) {
      selecting_ = true;

      QPoint cursor;
      if (!mapToGrid(event->pos(), cursor))
        return;

      if (!selection_) {
        selection_ = new Selection();
        scene()->addItem(selection_);
      }

      startPos_ = cursor;
      selection_->set(QRect(cursor, QSize(0, 0)));

      return;
    }

    Document *doc = GlobalState::self()->activeDocument();
    if (!GlobalState::self()->color() || !doc)
      return;

    drawmap_ = new SparseMap(doc);

    if (mode == ToolMode_Full || mode == ToolMode_Half ||
        mode == ToolMode_Petite || mode == ToolMode_Quarter) {
      QPoint cursor;
      if (!mapToGrid(event->pos(), cursor))
        return;

      if (selection_ && !selection_->within(cursor))
        return;

      drawing_ = true;
    } else if (mode == ToolMode_Erase) {
      erasing_ = true;
    } else if (mode == ToolMode_Rectangle) {
      QPoint cursor;
      if (!mapToGrid(event->pos(), cursor))
        return;

      if (selection_ && !selection_->within(cursor))
        return;

      rectangle_ = true;
      startPos_ = cursor;
      lastRect_ = QRect(cursor, cursor);
    }

    mouseMoveEvent(event);
  } else if (event->button() & Qt::RightButton) {
    dragging_ = true;
    lastPos_ = event->pos();
    event->accept();
    return;
  }

  event->ignore();
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
  const Color *c = GlobalState::self()->color();

  if (selecting_) {
    /* select */

    QPoint cursor;

    if (!mapToGrid(event->pos(), cursor))
        return;

    selection_->set(QRect(startPos_, cursor));
  } else if (dragging_) {
    /* pan */

    QPointF delta = mapToScene(lastPos_) - mapToScene(event->pos());
    lastPos_ = event->pos();
    setCenter(center_ + delta);
  } else if (drawing_) {
    Subarea subcursor;
    QPoint cursor;

    if (!mapToGrid(event->pos(), cursor, subcursor))
      return;

    if (selection_ && !selection_->within(cursor))
        return;

    ToolMode mode = GlobalState::self()->toolMode();
    
    if (cursor != cursor_) {
      /* draw */

      cursor_ = cursor;

      Cell *cell = drawmap_->cellAt(cursor);

      if (mode == ToolMode_Full) {
        cell->addFullStitch(c);
      } else {
        Orientation o;
        if (subcursor == Subarea_TopLeft ||
            subcursor == Subarea_BottomRight)
          o = Orientation_Backslash;
        else
          o = Orientation_Slash;

        subcursor_ = subcursor;
        if (mode == ToolMode_Half)
          cell->addHalfStitch(o, c);
        else if (mode == ToolMode_Petite)
          cell->addPetiteStitch(subcursor, c);
        else if (mode == ToolMode_Quarter)
          cell->addQuarterStitch(o, subcursor, c);
      }

      cell->createGraphicsItems();
    }
  } else if (erasing_) {
    /* erase */
    
    QPoint cursor;

    if (!mapToGrid(event->pos(), cursor))
      return;

    if (selection_ && !selection_->within(cursor))
        return;

    if (cursor != cursor_) {
      cursor_ = cursor;

      Document *doc = GlobalState::self()->activeDocument();
      SparseMap *map = doc->map();
      
      if (map->contains(cursor)) {
        Cell *c = map->cellAt(cursor);
        c->clearGraphicsItems();
        drawmap_->cellAt(cursor);
      }
    }
  } else if (rectangle_) {
    /* rect */

    QPoint cursor;

    if (!mapToGrid(event->pos(), cursor))
        return;

    if (selection_ && !selection_->within(cursor))
        return;
    
    QRect rect(startPos_, cursor);

    if (!rect.isValid())
        rect = rect.normalized();

    for (int y = rect.y(); y < rect.y() + rect.height(); ++y) {
      for (int x = rect.x(); x < rect.x() + rect.width(); ++x) {
        if (!drawmap_->contains(QPoint(x, y))) {
          Cell *cell = drawmap_->cellAt(QPoint(x, y));
          cell->addFullStitch(c);
          cell->createGraphicsItems();
        }
      }
    }

    if (rect.x() > lastRect_.x()) {
      for (int y = lastRect_.y(); y < lastRect_.y() + lastRect_.height(); ++y) {
        for (int x = lastRect_.x(); x < rect.x(); ++x) {
          drawmap_->remove(QPoint(x, y));
        }
      }
    }

    if (rect.y() > lastRect_.y()) {
      for (int y = lastRect_.y(); y < rect.y(); ++y) {
        for (int x = lastRect_.x(); x < lastRect_.x() + lastRect_.width(); ++x) {
          drawmap_->remove(QPoint(x, y));
        }
      }
    }

    if (rect.x() + rect.width() < lastRect_.x() + lastRect_.width()) {
      for (int y = lastRect_.y(); y < lastRect_.y() + lastRect_.height(); ++y) {
        for (int x = rect.x() + rect.width(); x < lastRect_.x() + lastRect_.width(); ++x) {
          drawmap_->remove(QPoint(x, y));
        }
      }
    }

    if (rect.y() + rect.height() < lastRect_.y() + lastRect_.height()) {
      for (int y = rect.y() + rect.height(); y < lastRect_.y() + lastRect_.height(); ++y) {
        for (int x = lastRect_.x(); x < lastRect_.x() + lastRect_.width(); ++x) {
          drawmap_->remove(QPoint(x, y));
        }
      }
    }

    lastRect_ = rect;
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
  if (dragging_ && event->button() & Qt::RightButton) {
    dragging_ = false;
  } else if ((drawing_ || rectangle_) && event->button() & Qt::LeftButton) {
    Document *doc = GlobalState::self()->activeDocument();
    if (!doc)
      return;
    
    drawing_ = false;
    rectangle_ = false;
    cursor_ = QPoint(-1, -1);
    subcursor_ = Subarea_TopLeft;
    doc->editor()->edit(new ActionDraw(doc, drawmap_));
    delete drawmap_;
  } else if (erasing_ && event->button() & Qt::LeftButton) {
    Document *doc = GlobalState::self()->activeDocument();
    if (!doc)
      return;

    erasing_ = false;
    cursor_ = QPoint(-1, -1);
    doc->editor()->edit(new ActionErase(doc, drawmap_));
    delete drawmap_;
  } else if (selecting_ && event->button() & Qt::LeftButton) {
    const QRect &r = selection_->rect();

    if (r.width() == 0 && r.height() == 0) {
      delete selection_;
      selection_ = NULL;
      emit clearedSelection();
    } else {
      emit madeSelection(r);
    }

    selecting_ = false;
  }
}

void Canvas::wheelEvent(QWheelEvent *event)
{
  double mag;

  if (event->delta() > 0) {
    mag = 1.0 + MAGNIFICATION_RATE;
  } else {
    mag = 1.0 - MAGNIFICATION_RATE;
  }

  scale(mag, mag);
}


