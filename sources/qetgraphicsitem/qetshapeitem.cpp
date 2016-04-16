/*
	Copyright 2006-2015 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetshapeitem.h"
#include "createdxf.h"
#include "diagram.h"
#include "qet.h"
#include "shapegraphicsitempropertieswidget.h"
#include "PropertiesEditor/propertieseditordialog.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "qetxml.h"

/**
 * @brief QetShapeItem::QetShapeItem
 * Constructor of shape item. point 1 and 2 must be in scene coordinate
 * @param p1 first point
 * @param p2 second point
 * @param type type of item (line, rectangle, ellipse)
 * @param parent parent item
 */
QetShapeItem::QetShapeItem(QPointF p1, QPointF p2, ShapeType type, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_shapeType(type),
	m_P1 (p1),
	m_P2 (p2),
	m_hovered(false),
	m_mouse_grab_handler(false),
	m_handler(10)
{
	if (type == Polygon) m_polygon << m_P1 << m_P2;
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
	m_pen.setStyle(Qt::DashLine);

}

QetShapeItem::~QetShapeItem() {}

/**
 * @brief QetShapeItem::setPen
 * Set the pen to use for draw the shape
 * @param pen
 */
void QetShapeItem::setPen(const QPen &pen)
{
	if (m_pen == pen) return;
	m_pen = pen;
	update();
	emit penChanged();
}

/**
 * @brief QetShapeItem::setBrush
 * Set the brush to use for the fill the shape
 * @param brush
 */
void QetShapeItem::setBrush(const QBrush &brush)
{
	if (m_brush == brush) return;
	m_brush = brush;
	update();
	emit brushChanged();
}

/**
 * @brief QetShapeItem::setP2
 * Set the second point of this item.
 * If this item is a polyline,
 * the last point of the polyline is replaced by P2.
 * @param P2
 */
void QetShapeItem::setP2(const QPointF &P2)
{
	if (m_shapeType == Polygon && m_polygon.last() != P2)
	{
		prepareGeometryChange();
		m_polygon.replace(m_polygon.size()-1, P2);
	}
	else if (P2 != m_P2)
	{
		prepareGeometryChange();
		m_P2 = P2;
	}
}

/**
 * @brief QetShapeItem::setLine
 * Set item geometry to line (only available for line shape)
 * @param line
 * @return  : true when shape is a Line, else false
 */
bool QetShapeItem::setLine(const QLineF &line)
{
	if (Q_UNLIKELY(m_shapeType != Line)) return false;
	prepareGeometryChange();
	m_P1 = line.p1();
	m_P2 = line.p2();
	return true;
}

/**
 * @brief QetShapeItem::setRect
 * Set this item geometry to rect (only available if shape is a rectangle or an ellipse)
 * @param rect : new rect
 * @return  : true when shape is rectangle or ellipse, else false
 */
bool QetShapeItem::setRect(const QRectF &rect)
{
	if (Q_LIKELY(m_shapeType == Rectangle || m_shapeType == Ellipse))
	{
		prepareGeometryChange();
		m_P1 = rect.topLeft();
		m_P2 = rect.bottomRight();
		return true;
	}

	return false;
}

/**
 * @brief QetShapeItem::setPolygon
 * Set this item geometry to polygon (only available if shape is a polyline)
 * @param polygon : new polygon
 * @return true if item is polygon, else false
 */
bool QetShapeItem::setPolygon(const QPolygonF &polygon)
{
	if (Q_UNLIKELY(m_shapeType != Polygon)) return false;
	prepareGeometryChange();
	m_polygon = polygon;
	return true;
}

/**
 * @brief QetShapeItem::setClosed
 * Close this item, have effect only if this item is a polygon.
 * @param close
 */
void QetShapeItem::setClosed(bool close)
{
	if (m_shapeType == Polygon && close != m_close)
	{
		prepareGeometryChange();
		m_close = close;
		emit closeChanged();
	}
}

/**
 * @brief QetShapeItem::pointCount
 * @return the number of point in the polygon
 */
int QetShapeItem::pointsCount() const {
	return m_polygon.size();
}

/**
 * @brief QetShapeItem::setNextPoint
 * Add a new point to the curent polygon
 * @param P the new point.
 */
void QetShapeItem::setNextPoint(QPointF P)
{
	prepareGeometryChange();
	m_polygon.append(Diagram::snapToGrid(P));
}

/**
 * @brief QetShapeItem::removePoints
 * Number of point to remove on the polygon
 * If @number is superior to number of polygon points-2,
 * all points of polygon will be removed except the first two (minimum point for the polygon);
 */
void QetShapeItem::removePoints(int number)
{
	if (pointsCount() == 2 || number < 1) return;
	if ((pointsCount()-2) < number)
		number = pointsCount() - 2;

	int i = 0;
	do
	{
		i++;
		prepareGeometryChange();
		m_polygon.pop_back();
		setTransformOriginPoint(boundingRect().center());

	} while (i < number);
}

/**
 * @brief QetShapeItem::boundingRect
 * @return the bounding rect of this item
 */
QRectF QetShapeItem::boundingRect() const {
	return shape().boundingRect().adjusted(-6, -6, 6, 6);
}

/**
 * @brief QetShapeItem::shape
 * @return the shape of this item
 */
QPainterPath QetShapeItem::shape() const
{
	QPainterPath path;

	switch (m_shapeType)
	{
		case Line:      
			path.moveTo(m_P1);
			path.lineTo(m_P2);                   
			break;
		case Rectangle: 
			path.addRect(QRectF(m_P1, m_P2));
			break;
		case Ellipse:
			path.addEllipse(QRectF(m_P1, m_P2));
			break;
		case Polygon:
			path.addPolygon(m_polygon);
			if (m_close) {
				path.closeSubpath();
			}
			break;
		default:
			Q_ASSERT(false);
			break;
	}

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? m_pen.widthF()+10 : m_pen.widthF());
	pps.setJoinStyle(Qt::RoundJoin);
	path = pps.createStroke(path);

	if (isSelected())
	{
		QVector <QPointF> vector;

		if (m_shapeType == Line)
			vector << m_P1 << m_P2;
		else if (m_shapeType == Rectangle || m_shapeType == Ellipse) {
			QRectF rect (m_P1, m_P2);
			vector << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft();
		}
		else
			vector = m_polygon;

		foreach(QRectF r, m_handler.handlerRect(vector))
			path.addRect(r);
	}

	return (path);
}

/**
 * @brief QetShapeItem::paint
 * Paint this item
 * @param painter
 * @param option
 * @param widget
 */
void QetShapeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option); Q_UNUSED(widget);

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setPen(m_pen);
	painter->setBrush(m_brush);

		//Draw hovered shadow
	if (m_hovered)
	{
		painter->save();
		QColor color(Qt::darkBlue);
		color.setAlpha(25);
		painter -> setBrush (QBrush (color));
		painter -> setPen   (Qt::NoPen);
		painter -> drawPath (shape());
		painter -> restore  ();
	}

		//Draw the shape and handlers if is selected
	switch (m_shapeType)
	{
		case Line:
			painter->drawLine(QLineF(m_P1, m_P2));
			if (isSelected())
				m_handler.drawHandler(painter, QVector<QPointF>{m_P1, m_P2});
			break;

		case Rectangle:
			painter->drawRect(QRectF(m_P1, m_P2));
			if (isSelected())
				m_handler.drawHandler(painter, m_handler.pointsForRect(QRectF(m_P1, m_P2)));
			break;

		case Ellipse:
			painter->drawEllipse(QRectF(m_P1, m_P2));
			if (isSelected())
				m_handler.drawHandler(painter, m_handler.pointsForRect(QRectF(m_P1, m_P2)));
			break;

		case Polygon:
			m_close ? painter->drawPolygon(m_polygon) : painter->drawPolyline(m_polygon);
			if (isSelected())
				m_handler.drawHandler(painter, m_polygon);
			break;
	}
	painter->restore();
}

/**
 * @brief QetShapeItem::hoverEnterEvent
 * Handle hover enter event
 * @param event
 */
void QetShapeItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
	m_hovered = true;
	QetGraphicsItem::hoverEnterEvent(event);
}

void QetShapeItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	if (!isSelected()) return;

	QVector <QPointF> vector;
	switch (m_shapeType)
	{
		case Line:      vector << m_P1 << m_P2;                               break;
		case Rectangle: vector = m_handler.pointsForRect(QRectF(m_P1, m_P2)); break;
		case Ellipse:   vector = m_handler.pointsForRect(QRectF(m_P1, m_P2)); break;
		case Polygon:   vector = m_polygon;                                   break;
	}

	int handler = m_handler.pointIsHoverHandler(event->pos(), vector);
	if (handler >= 0)
	{
		if (m_shapeType & (Line | Polygon)) {
			setCursor(Qt::SizeAllCursor);
			return;
		}

		if (handler == 0 || handler == 2 || handler == 5 || handler == 7)
			setCursor(Qt::SizeAllCursor);
		else if (handler == 1 || handler == 6)
			setCursor(Qt::SizeVerCursor);
		else if (handler == 3 || handler == 4)
			setCursor(Qt::SizeHorCursor);
	}
	else
		setCursor(Qt::OpenHandCursor);
}

/**
 * @brief QetShapeItem::hoverLeaveEvent
 * Handle hover leave event
 * @param event
 */
void QetShapeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
	m_hovered = false;
	unsetCursor();
	QetGraphicsItem::hoverLeaveEvent(event);
}

/**
 * @brief QetShapeItem::mousePressEvent
 * Handle mouse press event
 * @param event
 */
void QetShapeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		setCursor(Qt::ClosedHandCursor);
			//Shape is selected, we see if user click in a handler
		if (isSelected())
		{
			QVector <QPointF> vector;
			switch (m_shapeType)
			{
				case Line:      vector << m_P1 << m_P2;                               break;
				case Rectangle: vector = m_handler.pointsForRect(QRectF(m_P1, m_P2)); break;
				case Ellipse:   vector = m_handler.pointsForRect(QRectF(m_P1, m_P2)); break;
				case Polygon:  vector = m_polygon;                                   break;
			}

			m_vector_index = m_handler.pointIsHoverHandler(event->pos(), vector);
			if (m_vector_index != -1)
			{
					//User click on an handler
				m_mouse_grab_handler = true;
				m_old_P1 = m_P1;
				m_old_P2 = m_P2;
				m_old_polygon = m_polygon;
				return;
			}
		}
	}

	QetGraphicsItem::mousePressEvent(event);
}

/**
 * @brief QetShapeItem::mouseMoveEvent
 * Handle move event
 * @param event
 */
void QetShapeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_mouse_grab_handler)
	{
		QPointF new_pos = event->pos();
		if (event->modifiers() != Qt::ControlModifier)
			new_pos = mapFromScene(Diagram::snapToGrid(event->scenePos()));

		switch (m_shapeType)
		{
			case Line:
				prepareGeometryChange();
				m_vector_index == 0 ? m_P1 = new_pos : m_P2 = new_pos;
				break;

			case Rectangle: setRect(m_handler.rectForPosAtIndex(QRectF(m_P1, m_P2), new_pos, m_vector_index)); break;
			case Ellipse:   setRect(m_handler.rectForPosAtIndex(QRectF(m_P1, m_P2), new_pos, m_vector_index)); break;

			case Polygon:
				prepareGeometryChange();
				m_polygon.replace(m_vector_index, new_pos);
				break;
		}	//End switch
		return;
	}

	QetGraphicsItem::mouseMoveEvent(event);
}

/**
 * @brief QetShapeItem::mouseReleaseEvent
 * Handle mouse release event
 * @param event
 */
void QetShapeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_mouse_grab_handler)
	{
		m_mouse_grab_handler = false;
		if (diagram())
		{
			QPropertyUndoCommand *undo = nullptr;
			if ((m_shapeType & (Line | Rectangle | Ellipse)) && (m_P1 != m_old_P1 || m_P2 != m_old_P2))
			{
				switch(m_shapeType)
				{
					case Line:      undo = new QPropertyUndoCommand(this, "line",QLineF(m_old_P1, m_old_P2), QLineF(m_P1, m_P2)); break;
					case Rectangle: undo = new QPropertyUndoCommand(this, "rect",QRectF(m_old_P1, m_old_P2), QRectF(m_P1, m_P2)); break;
					case Ellipse:   undo = new QPropertyUndoCommand(this, "rect",QRectF(m_old_P1, m_old_P2), QRectF(m_P1, m_P2)); break;
					case Polygon: break;
				}
				if (undo) undo->enableAnimation();
			}
			else if (m_shapeType == Polygon && (m_polygon != m_old_polygon))
				undo = new QPropertyUndoCommand(this, "polygon", m_old_polygon, m_polygon);

			if(undo)
			{
				undo->setText(tr("Modifier %1").arg(name()));
				diagram()->undoStack().push(undo);
			}
		}
		setCursor(Qt::OpenHandCursor);
	}

	QetGraphicsItem::mouseReleaseEvent(event);
}

/**
 * @brief QetShapeItem::fromXml
 * Build this item from the xml description
 * @param e element where is stored this item
 * @return true if load success
 */
bool QetShapeItem::fromXml(const QDomElement &e)
{
	if (e.tagName() != "shape") return (false);

	is_movable_ = (e.attribute("is_movable").toInt());
	m_close = e.attribute("closed", "0").toInt();
	m_pen = QETXML::penFromXml(e.firstChildElement("pen"));
	m_brush = QETXML::brushFromXml(e.firstChildElement("brush"));

	QString type = e.attribute("type");
		//@TODO Compatibility for version older than N°4075, shape type was stored with an int
	if (type.size() == 1)
	{
		switch(e.attribute("type","0").toInt())
		{
			case 0: m_shapeType = Line;      break;
			case 1: m_shapeType = Rectangle; break;
			case 2: m_shapeType = Ellipse;   break;
			case 3: m_shapeType = Polygon;  break;
		}
	}
		//For version after N°4075, shape is stored with a string
	else
	{
		QMetaEnum me = metaObject()->enumerator(metaObject()->indexOfEnumerator("ShapeType"));
		m_shapeType = QetShapeItem::ShapeType(me.keysToValue(type.toStdString().data()));
	}

	if (m_shapeType != Polygon)
	{
		m_P1.setX(e.attribute("x1", 0).toDouble());
		m_P1.setY(e.attribute("y1", 0).toDouble());
		m_P2.setX(e.attribute("x2", 0).toDouble());
		m_P2.setY(e.attribute("y2", 0).toDouble());
	}
	else
		foreach(QDomElement de, QET::findInDomElement(e, "points", "point"))
			m_polygon << QPointF(de.attribute("x", 0).toDouble(), de.attribute("y", 0).toDouble());

	return (true);
}

/**
 * @brief QetShapeItem::toXml
 * Save this item to xml element
 * @param document parent document xml
 * @return element xml where is write this item
 */
QDomElement QetShapeItem::toXml(QDomDocument &document) const
{
	QDomElement result = document.createElement("shape");

		//write some attribute
	QMetaEnum me = metaObject()->enumerator(metaObject()->indexOfEnumerator("ShapeType"));
	result.setAttribute("type", me.valueToKey(m_shapeType));
	result.appendChild(QETXML::penToXml(document, m_pen));
	result.appendChild(QETXML::brushToXml(document, m_brush));
	result.setAttribute("is_movable", bool(is_movable_));
	result.setAttribute("closed", bool(m_close));

	if (m_shapeType != Polygon)
	{
		result.setAttribute("x1", QString::number(mapToScene(m_P1).x()));
		result.setAttribute("y1", QString::number(mapToScene(m_P1).y()));
		result.setAttribute("x2", QString::number(mapToScene(m_P2).x()));
		result.setAttribute("y2", QString::number(mapToScene(m_P2).y()));
	}
	else
	{
		QDomElement points = document.createElement("points");
		foreach(QPointF p, m_polygon)
		{
			QDomElement point = document.createElement("point");
			QPointF pf = mapToScene(p);
			point.setAttribute("x", pf.x());
			point.setAttribute("y", pf.y());
			points.appendChild(point);
		}
		result.appendChild(points);
	}

	return(result);
}

/**
 * @brief QetShapeItem::toDXF
 * Draw this element to the dxf document
 * @param filepath file path of the the dxf document
 * @return true if draw success
 */
bool QetShapeItem::toDXF(const QString &filepath)
{
	switch (m_shapeType)
	{
		case Line:      Createdxf::drawLine     (filepath, QLineF(mapToScene(m_P1), mapToScene(m_P2)), 0);              return true;
		case Rectangle: Createdxf::drawRectangle(filepath, QRectF(mapToScene(m_P1), mapToScene(m_P2)).normalized(), 0); return true;
		case Ellipse:   Createdxf::drawEllipse  (filepath, QRectF(mapToScene(m_P1), mapToScene(m_P2)).normalized(), 0); return true;
		default: return false;
	}
}

/**
 * @brief QetShapeItem::editProperty
 * Edit the property of this item
 */
void QetShapeItem::editProperty()
{
	if (diagram() -> isReadOnly()) return;

	PropertiesEditorDialog ped(new ShapeGraphicsItemPropertiesWidget(this), diagram()->views().at(0));
	ped.exec();
}

/**
 * @brief QetShapeItem::name
 * @return the name of the curent shape.
 */
QString QetShapeItem::name() const {
	switch (m_shapeType) {
		case Line:	    return tr("une ligne");
		case Rectangle:	return tr("un rectangle");
		case Ellipse:	return tr("une éllipse");
		case Polygon:	return tr("une polyligne");
		default:	    return tr("une shape");
	}
}
