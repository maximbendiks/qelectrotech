/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "elementscene.h"
#include "qetelementeditor.h"
#include <cmath>
#include "partline.h"
#include "partrectangle.h"
#include "partellipse.h"
#include "partcircle.h"
#include "partpolygon.h"
#include "partterminal.h"
#include "parttext.h"
#include "parttextfield.h"
#include "partarc.h"
#include "hotspoteditor.h"
#include "editorcommands.h"
#include "elementcontent.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent le Widget parent
*/
ElementScene::ElementScene(QETElementEditor *editor, QObject *parent) :
	QGraphicsScene(parent),
	_width(3),
	_height(7),
	_hotspot(15, 35),
	internal_connections(false),
	qgi_manager(this),
	element_editor(editor)
{
	current_polygon = NULL;
	setGrid(1, 1);
	initPasteArea();
	undo_stack.setClean();
}

/// Destructeur
ElementScene::~ElementScene() {
}

/**
	Passe la scene en mode "selection et deplacement de parties"
*/
void ElementScene::slot_move() {
	behavior = Normal;
}

/**
	Passe la scene en mode "ajout de ligne"
*/
void ElementScene::slot_addLine() {
	behavior = Line;
}

/**
	Passe la scene en mode "ajout de rectangle"
*/
void ElementScene::slot_addRectangle() {
	behavior = Rectangle;
}

/**
	Passe la scene en mode "ajout de cercle"
*/
void ElementScene::slot_addCircle() {
	behavior = Circle;
}

/**
	Passe la scene en mode "ajout d'ellipse"
*/
void ElementScene::slot_addEllipse() {
	behavior = Ellipse;
}

/**
	Passe la scene en mode "ajout de polygone"
*/
void ElementScene::slot_addPolygon() {
	behavior = Polygon;
}


/**
	Passe la scene en mode "ajout de texte statique"
*/
void ElementScene::slot_addText() {
	behavior = Text;
}

/**
	Passe la scene en mode "ajout de borne"
*/
void ElementScene::slot_addTerminal() {
	behavior = Terminal;
}

/**
	Passe la scene en mode "ajout d'arc de cercle"
*/
void ElementScene::slot_addArc() {
	behavior = Arc;
}

/**
	Passe la scene en mode "ajout de champ de texte"
*/
void ElementScene::slot_addTextField() {
	behavior = TextField;
}

/**
	Gere les mouvements de la souris
	@param e objet decrivant l'evenement
*/
void ElementScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	QPointF event_pos = e -> scenePos();
	if (mustSnapToGrid(e)) snapToGrid(event_pos);
	
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	if (behavior == PasteArea) {
		QRectF current_rect(paste_area_ -> rect());
		current_rect.moveCenter(event_pos);
		paste_area_ -> setRect(current_rect);
		return;
	}
	
	QRectF temp_rect;
	qreal radius;
	QPointF temp_point;
	QPolygonF temp_polygon;
	if (e -> buttons() & Qt::LeftButton) {
		switch(behavior) {
			case Line:
				current_line -> setLine(QLineF(current_line -> line().p1(), event_pos));
				break;
			case Rectangle:
				temp_rect = current_rectangle -> rect();
				temp_rect.setBottomRight(event_pos);
				current_rectangle -> setRect(temp_rect);
				break;
			case Ellipse:
				temp_rect = current_ellipse -> rect();
				temp_rect.setBottomRight(event_pos);
				current_ellipse -> setRect(temp_rect);
				break;
			case Arc:
				temp_rect = current_arc -> rect();
				temp_rect.setBottomRight(event_pos);
				current_arc -> setRect(temp_rect);
				break;
			case Circle:
				temp_rect = current_circle -> rect();
				temp_point = event_pos - current_circle -> mapToScene(temp_rect.center());
				radius = sqrt(pow(temp_point.x(), 2) + pow(temp_point.y(), 2));
				temp_rect = QRectF(
					temp_rect.center() - QPointF(radius, radius),
					QSizeF(2.0 * radius, 2.0 * radius)
				);
				current_circle -> setRect(temp_rect);
				break;
			case Polygon:
				if (current_polygon == NULL) break;
				temp_polygon = current_polygon -> polygon();
				temp_polygon.pop_back();
				temp_polygon << event_pos;
				current_polygon -> setPolygon(temp_polygon);
				break;
			case Normal:
			default:
				QList<QGraphicsItem *> selected_items = selectedItems();
				if (!selected_items.isEmpty()) {
					// mouvement de souris realise depuis le dernier press event
					QPointF mouse_movement = e -> scenePos() - moving_press_pos;
					
					// application de ce mouvement a la fsi_pos enregistre dans le dernier press event
					QPointF new_fsi_pos = fsi_pos + mouse_movement;
					
					// snap eventuel de la nouvelle fsi_pos
					if (mustSnapToGrid(e)) snapToGrid(new_fsi_pos);
					
					// difference entre la fsi_pos finale et la fsi_pos courante = mouvement a appliquer
					
					QPointF current_fsi_pos = selected_items.first() -> scenePos();
					QPointF final_movement = new_fsi_pos - current_fsi_pos;
					foreach(QGraphicsItem *qgi, selected_items) {
						qgi -> moveBy(final_movement.x(), final_movement.y());
					}
				}
		}
	} else if (behavior == Polygon && current_polygon != NULL) {
		temp_polygon = current_polygon -> polygon();
		temp_polygon.pop_back();
		temp_polygon << event_pos;
		current_polygon -> setPolygon(temp_polygon);
	} else QGraphicsScene::mouseMoveEvent(e);
}

/**
	Gere les appuis sur les boutons de la souris
	@param e objet decrivant l'evenement
*/
void ElementScene::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	QPointF event_pos = e -> scenePos();
	if (mustSnapToGrid(e)) snapToGrid(event_pos);
	
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	QPolygonF temp_polygon;
	if (e -> button() & Qt::LeftButton) {
		switch(behavior) {
			case Line:
				current_line = new PartLine(element_editor, 0, this);
				current_line -> setLine(QLineF(event_pos, event_pos));
				break;
			case Rectangle:
				current_rectangle = new PartRectangle(element_editor, 0, this);
				current_rectangle -> setRect(QRectF(event_pos, QSizeF(0.0, 0.0)));
				break;
			case Ellipse:
				current_ellipse = new PartEllipse(element_editor, 0, this);
				current_ellipse -> setRect(QRectF(event_pos, QSizeF(0.0, 0.0)));
				current_ellipse -> setProperty("antialias", true);
				break;
			case Arc:
				current_arc = new PartArc(element_editor, 0, this);
				current_arc -> setRect(QRectF(event_pos, QSizeF(0.0, 0.0)));
				current_arc -> setProperty("antialias", true);
				break;
			case Circle:
				current_circle = new PartCircle(element_editor, 0, this);
				current_circle -> setRect(QRectF(event_pos, QSizeF(0.0, 0.0)));
				current_circle -> setProperty("antialias", true);
				break;
			case Polygon:
				if (current_polygon == NULL) {
					current_polygon = new PartPolygon(element_editor, 0, this);
					temp_polygon = QPolygonF(0);
				} else temp_polygon = current_polygon -> polygon();
				// au debut, on insere deux points
				if (!temp_polygon.count()) temp_polygon << event_pos;
				temp_polygon << event_pos;
				current_polygon -> setPolygon(temp_polygon);
				break;
			case Normal:
			default:
				QGraphicsScene::mousePressEvent(e);
				// gestion des deplacements de parties
				if (!selectedItems().isEmpty()) {
					fsi_pos = selectedItems().first() -> scenePos();
					moving_press_pos = e -> scenePos();
					moving_parts_ = true;
				} else {
					fsi_pos = QPoint();
					moving_press_pos = QPoint();
					moving_parts_ = false;
				}
		}
	} else QGraphicsScene::mousePressEvent(e);
}

/**
	Gere les relachements de boutons de la souris
	@param e objet decrivant l'evenement
*/
void ElementScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	QPointF event_pos = e -> scenePos();
	if (mustSnapToGrid(e)) snapToGrid(event_pos);
	
	PartTerminal *terminal;
	PartText *text;
	PartTextField *textfield;
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	
	if (behavior == PasteArea) {
		defined_paste_area_ = paste_area_ -> rect();
		removeItem(paste_area_);
		emit(pasteAreaDefined(defined_paste_area_));
		behavior = Normal;
		return;
	}
	
	if (e -> button() & Qt::LeftButton) {
		switch(behavior) {
			case Line:
				if (qgiManager().manages(current_line)) break;
				undo_stack.push(new AddPartCommand(tr("ligne"), this, current_line));
				emit(partsAdded());
				endCurrentBehavior(e);
				break;
			case Rectangle:
				if (qgiManager().manages(current_rectangle)) break;
				current_rectangle -> setRect(current_rectangle -> rect().normalized());
				undo_stack.push(new AddPartCommand(tr("rectangle"), this, current_rectangle));
				emit(partsAdded());
				endCurrentBehavior(e);
				break;
			case Ellipse:
				if (qgiManager().manages(current_ellipse)) break;
				current_ellipse -> setRect(current_ellipse -> rect().normalized());
				undo_stack.push(new AddPartCommand(tr("ellipse"), this, current_ellipse));
				emit(partsAdded());
				endCurrentBehavior(e);
				break;
			case Arc:
				if (qgiManager().manages(current_arc)) break;
				current_arc-> setRect(current_arc -> rect().normalized());
				undo_stack.push(new AddPartCommand(tr("arc"), this, current_arc));
				emit(partsAdded());
				endCurrentBehavior(e);
				break;
			case Circle:
				if (qgiManager().manages(current_circle)) break;
				current_circle -> setRect(current_circle -> rect().normalized());
				undo_stack.push(new AddPartCommand(tr("cercle"), this, current_circle));
				emit(partsAdded());
				endCurrentBehavior(e);
				break;
			case Terminal:
				terminal = new PartTerminal(element_editor, 0, this);
				terminal -> setPos(event_pos);
				undo_stack.push(new AddPartCommand(tr("borne"), this, terminal));
				emit(partsAdded());
				endCurrentBehavior(e);
				break;
			case Text:
				text = new PartText(element_editor, 0, this);
				text -> setPos(event_pos);
				undo_stack.push(new AddPartCommand(tr("texte"), this, text));
				emit(partsAdded());
				endCurrentBehavior(e);
				break;
			case TextField:
				textfield = new PartTextField(element_editor, 0, this);
				textfield -> setPos(event_pos);
				undo_stack.push(new AddPartCommand(tr("champ de texte"), this, textfield));
				emit(partsAdded());
				endCurrentBehavior(e);
				break;
			case Normal:
			default:
				// detecte les deplacements de parties
				if (!selectedItems().isEmpty() && moving_parts_) {
					QPointF movement = selectedItems().first() -> scenePos() - fsi_pos;
					if (!movement.isNull()) {
						undo_stack.push(new MovePartsCommand(movement, this, selectedItems()));
					}
				}
				QGraphicsScene::mouseReleaseEvent(e);
				moving_parts_ = false;
		}
	} else if (e -> button() & Qt::RightButton) {
		if (behavior == Polygon && current_polygon != NULL) {
			undo_stack.push(new AddPartCommand(tr("polygone"), this, current_polygon));
			current_polygon = NULL;
			emit(partsAdded());
			endCurrentBehavior(e);
		} else QGraphicsScene::mouseReleaseEvent(e);
	} else QGraphicsScene::mouseReleaseEvent(e);
}

/**
	Dessine l'arriere-plan de l'editeur, cad la grille.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void ElementScene::drawBackground(QPainter *p, const QRectF &r) {
	p -> save();
	
	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	p -> setBrush(Qt::white);
	p -> drawRect(r);
	
	// encadre la zone dessinable de l'element
	QRectF drawable_area(-_hotspot.x(), -_hotspot.y(), width(), height());
	p -> setPen(Qt::black);
	p -> setBrush(Qt::NoBrush);
	p -> drawRect(drawable_area);
	
	// on dessine un point de la grille sur 10
	int drawn_x_grid = x_grid * 10;
	int drawn_y_grid = y_grid * 10;
	
	if (r.width() < 2500 && r.height() < 2500) {
		// dessine les points de la grille
		p -> setPen(Qt::black);
		p -> setBrush(Qt::NoBrush);
		qreal limite_x = r.x() + r.width();
		qreal limite_y = r.y() + r.height();
		
		int g_x = (int)ceil(r.x());
		while (g_x % drawn_x_grid) ++ g_x;
		int g_y = (int)ceil(r.y());
		while (g_y % drawn_y_grid) ++ g_y;
		
		for (int gx = g_x ; gx < limite_x ; gx += drawn_x_grid) {
			for (int gy = g_y ; gy < limite_y ; gy += drawn_y_grid) {
				p -> drawPoint(gx, gy);
			}
		}
	}
	p -> restore();
}

/**
	Dessine l'arriere-plan de l'editeur, cad l'indicateur de hotspot.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void ElementScene::drawForeground(QPainter *p, const QRectF &) {
	p -> save();
	
	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	p -> setPen(Qt::red);
	p -> setBrush(Qt::NoBrush);
	p -> drawLine(QLineF(0.0, -_hotspot.y(), 0.0, height() - _hotspot.y()));
	p -> drawLine(QLineF(-_hotspot.x(), 0.0, width() - _hotspot.x(),  0.0));
	p -> restore();
}

/**
	A partir d'un evenement souris, cette methode regarde si la touche shift est
	enfoncee ou non. Si oui, elle laisse le comportement en cours (cercle,
	texte, polygone, ...). Si non, elle repasse en mode normal / selection.
	@param e objet decrivant l'evenement souris
*/
void ElementScene::endCurrentBehavior(const QGraphicsSceneMouseEvent *event) {
	if (!(event -> modifiers() & Qt::ShiftModifier)) {
		// la touche Shift n'est pas enfoncee : on demande le mode normal
		behavior = Normal;
		emit(needNormalMode());
	}
}

/**
	@return la taille horizontale de la grille
*/
int ElementScene::xGrid() const {
	return(x_grid);
}

/**
	@return la taille verticale de la grille
*/
int ElementScene::yGrid() const {
	return(y_grid);
}

/**
	@param x_grid Taille horizontale de la grille
	@param y_grid Taille verticale de la grille
*/
void ElementScene::setGrid(int x_g, int y_g) {
	x_grid = x_g ? x_g : 1;
	y_grid = y_g ? y_g : 1;
}

/**
	Exporte l'element en XML
	@param diagram Booleen (a vrai par defaut) indiquant si le XML genere doit
	representer tout l'element ou seulement les elements selectionnes
	@return un document XML decrivant l'element
*/
const QDomDocument ElementScene::toXml(bool all_parts) const {
	// document XML
	QDomDocument xml_document;
	
	// racine du document XML
	QDomElement root = xml_document.createElement("definition");
	root.setAttribute("type",        "element");
	root.setAttribute("width",       QString("%1").arg(_width  * 10));
	root.setAttribute("height",      QString("%1").arg(_height * 10));
	root.setAttribute("hotspot_x",   QString("%1").arg(_hotspot.x()));
	root.setAttribute("hotspot_y",   QString("%1").arg(_hotspot.y()));
	root.setAttribute("orientation", ori.toString());
	root.setAttribute("version", QET::version);
	if (internal_connections) root.setAttribute("ic", "true");
	
	// noms de l'element
	root.appendChild(_names.toXml(xml_document));
	
	QDomElement description = xml_document.createElement("description");
	// description de l'element
	foreach(QGraphicsItem *qgi, zItems(true)) {
		// si l'export ne concerne que la selection, on ignore les parties non selectionnees
		if (!all_parts && !qgi -> isSelected()) continue;
		if (CustomElementPart *ce = dynamic_cast<CustomElementPart *>(qgi)) {
			if (ce -> isUseless()) continue;
			description.appendChild(ce -> toXml(xml_document));
		}
	}
	root.appendChild(description);
	
	xml_document.appendChild(root);
	return(xml_document);
}

/**
	@param xml_document un document XML decrivant un element
	@return le boundingRect du contenu de l'element
*/
QRectF ElementScene::boundingRectFromXml(const QDomDocument &xml_document) {
	// charge les parties depuis le document XML
	ElementContent loaded_content = loadContent(xml_document);
	if (loaded_content.isEmpty()) return(QRectF());
	
	// calcule le boundingRect
	QRectF bounding_rect = elementContentBoundingRect(loaded_content);
	
	// detruit les parties chargees
	qDeleteAll(loaded_content);
	
	return(bounding_rect);
}

/**
	Importe l'element decrit dans un document XML. Si une position est
	precisee, les elements importes sont positionnes de maniere a ce que le
	coin superieur gauche du plus petit rectangle pouvant les entourant tous
	(le bounding rect) soit a cette position.
	@param xml_document un document XML decrivant l'element
	@param position La position des parties importees
	@param consider_informations Si vrai, les informations complementaires
	(dimensions, hotspot, etc.) seront prises en compte
	@param content_ptr si ce pointeur vers un ElementContent est different de 0,
	il sera rempli avec le contenu ajoute a l'element par le fromXml
	@return true si l'import a reussi, false sinon
*/
void ElementScene::fromXml(
	const QDomDocument &xml_document,
	const QPointF &position,
	bool consider_informations,
	ElementContent *content_ptr
) {
	QString error_message;
	bool state = true;
	
	// prend en compte les informations de l'element
	if (consider_informations) {
		state = applyInformations(xml_document, &error_message);
	}
	
	// parcours des enfants de la definition : parties de l'element
	if (state) {
		ElementContent loaded_content = loadContent(xml_document, &error_message);
		if (position != QPointF()) {
			addContentAtPos(loaded_content, position, &error_message);
		} else {
			addContent(loaded_content, &error_message);
		}
		
		// renvoie le contenu ajoute a l'element
		if (content_ptr) {
			*content_ptr = loaded_content;
		}
	}
}

/**
	@return le rectangle representant les limites de l'element.
	Ce rectangle a pour dimensions la taille de l'element et pour coin
	superieur gauche les coordonnees opposees du hotspot.
*/
QRectF ElementScene::borderRect() const {
	return(QRectF(-_hotspot, QSizeF(width(), height())));
}

/**
	@return un rectangle englobant toutes les parties ainsi que le
	"bounding rect" de l'element
*/
QRectF ElementScene::sceneContent() const {
	qreal adjustment = 5.0;
	return(itemsBoundingRect().unite(borderRect()).adjusted(-adjustment, -adjustment, adjustment, adjustment));
}

/**
	@return true si toutes les parties graphiques composant l'element sont
	integralement contenues dans le rectangle representant les limites de
	l'element.
*/
bool ElementScene::borderContainsEveryParts() const {
	return(borderRect().contains(itemsBoundingRect()));
}

/**
	@return la pile d'annulations de cet editeur d'element
*/
QUndoStack &ElementScene::undoStack() {
	return(undo_stack);
}

/**
	@return le gestionnaire de QGraphicsItem de cet editeur d'element
*/
QGIManager &ElementScene::qgiManager() {
	return(qgi_manager);
}

/**
	@return true si le presse-papier semble contenir un element
*/
bool ElementScene::clipboardMayContainElement() {
	QString clipboard_text = QApplication::clipboard() -> text().trimmed();
	bool may_be_element = clipboard_text.startsWith("<definition") && clipboard_text.endsWith("</definition>");
	return(may_be_element);
}

/**
	@param clipboard_content chaine de caractere, provenant vraisemblablement du
	presse-papier.
	@return true si clipboard_content a ete copie depuis cet element.
*/
bool ElementScene::wasCopiedFromThisElement(const QString &clipboard_content) {
	return(clipboard_content == last_copied_);
}

/**
	Gere le fait de couper la selection = l'exporter en XML dans le
	presse-papier puis la supprimer.
*/
void ElementScene::cut() {
	copy();
	QList<QGraphicsItem *> cut_content = selectedItems();
	clearSelection();
	undoStack().push(new CutPartsCommand(this, cut_content));
}

/**
	Gere le fait de copier la selection = l'exporter en XML dans le
	presse-papier.
*/
void ElementScene::copy() {
	// accede au presse-papier
	QClipboard *clipboard = QApplication::clipboard();
	
	// genere la description XML de la selection
	QString clipboard_content = toXml(false).toString(4);
	
	// met la description XML dans le presse-papier
	if (clipboard -> supportsSelection()) {
		clipboard -> setText(clipboard_content, QClipboard::Selection);
	}
	clipboard -> setText(clipboard_content);
	
	// retient le dernier contenu copie
	last_copied_ = clipboard_content;
}

/**
	Gere le fait de coller le contenu du presse-papier = l'importer dans le
	presse-papier a une position donnee.
*/
void ElementScene::paste() {
	
}

/**
	Selectionne une liste de parties
	@param content liste des parties a selectionner
*/
void ElementScene::slot_select(const ElementContent &content) {
	blockSignals(true);
	foreach(QGraphicsItem *qgi, content) qgi -> setSelected(true);
	blockSignals(false);
	emit(selectionChanged());
}

/**
	Selectionne tout
*/
void ElementScene::slot_selectAll() {
	slot_select(items());
}

/**
	Deselectionne tout
*/
void ElementScene::slot_deselectAll() {
	clearSelection();
}

/**
	Inverse la selection
*/
void ElementScene::slot_invertSelection() {
	blockSignals(true);
	foreach(QGraphicsItem *qgi, items()) qgi -> setSelected(!qgi -> isSelected());
	blockSignals(false);
	emit(selectionChanged());
}

/**
	Supprime les elements selectionnes
*/
void ElementScene::slot_delete() {
	// verifie qu'il y a qqc de selectionne
	QList<QGraphicsItem *> selected_items = selectedItems();
	if (selected_items.isEmpty()) return;
	
	// efface tout ce qui est selectionne
	undo_stack.push(new DeletePartsCommand(this, selected_items));
	emit(partsRemoved());
}

/**
	Lance un dialogue pour editer les dimensions et le point de saisie
	(hotspot) de l'element.
*/
void ElementScene::slot_editSizeHotSpot() {
	// cree un dialogue
	QDialog dialog_sh(element_editor);
	dialog_sh.setModal(true);
	dialog_sh.setWindowTitle(tr("\311diter la taille et le point de saisie", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog_sh);
	
	// ajoute un HotspotEditor au dialogue
	HotspotEditor *hotspot_editor = new HotspotEditor();
	hotspot_editor -> setElementWidth(static_cast<uint>(width() / 10));
	hotspot_editor -> setElementHeight(static_cast<uint>(height() / 10));
	hotspot_editor -> setHotspot(hotspot());
	hotspot_editor -> setOldHotspot(hotspot());
	hotspot_editor -> setPartsRect(itemsBoundingRect());
	hotspot_editor -> setPartsRectEnabled(true);
	dialog_layout -> addWidget(hotspot_editor);
	
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),    &dialog_sh, SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),    &dialog_sh, SLOT(reject()));
	
	// lance le dialogue
	if (dialog_sh.exec() != QDialog::Accepted) return;
	QSize new_size(hotspot_editor -> elementSize());
	QSize old_size(width(), height());
	QPoint new_hotspot(hotspot_editor -> hotspot());
	QPoint old_hotspot(_hotspot);
	
	if (new_size != old_size || new_hotspot != old_hotspot) {
		undo_stack.push(new ChangeHotspotCommand(this, old_size, new_size, old_hotspot, new_hotspot, hotspot_editor -> offsetParts()));
	}
}

/**
	Lance un dialogue pour editer les noms de cete element
*/
void ElementScene::slot_editOrientations() {
	
	// cree un dialogue
	QDialog dialog_ori(element_editor);
	dialog_ori.setModal(true);
	dialog_ori.setMinimumSize(400, 260);
	dialog_ori.setWindowTitle(tr("\311diter les orientations", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog_ori);
	
	// ajoute un champ explicatif au dialogue
	QLabel *information_label = new QLabel(tr("L'orientation par d\351faut est l'orientation dans laquelle s'effectue la cr\351ation de l'\351l\351ment."));
	information_label -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	information_label -> setWordWrap(true);
	dialog_layout -> addWidget(information_label);
	
	// ajoute un OrientationSetWidget au dialogue
	OrientationSetWidget *ori_widget = new OrientationSetWidget();
	ori_widget -> setOrientationSet(ori);
	dialog_layout -> addWidget(ori_widget);
	
	// ajoute une case a cocher pour les connexions internes
	QCheckBox *ic_checkbox = new QCheckBox(tr("Autoriser les connexions internes"));
	ic_checkbox -> setChecked(internal_connections);
	dialog_layout -> addWidget(ic_checkbox);
	dialog_layout -> addStretch();
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),    &dialog_ori, SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),    &dialog_ori, SLOT(reject()));
	
	// lance le dialogue
	if (dialog_ori.exec() == QDialog::Accepted) {
		OrientationSet new_ori = ori_widget -> orientationSet();
		if (new_ori != ori) {
			undoStack().push(new ChangeOrientationsCommand(this, ori, new_ori));
		}
		if (ic_checkbox -> isChecked() != internal_connections) {
			undoStack().push(new AllowInternalConnectionsCommand(this, ic_checkbox -> isChecked()));
		}
	}
}

/**
	Lance un dialogue pour editer les noms de cet element
*/
void ElementScene::slot_editNames() {
	
	// cree un dialogue
	QDialog dialog(element_editor);
	dialog.setModal(true);
	dialog.setMinimumSize(400, 330);
	dialog.setWindowTitle(tr("\311diter les noms", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog);
	
	// ajoute un champ explicatif au dialogue
	QLabel *information_label = new QLabel(tr("Vous pouvez sp\351cifier le nom de l'\351l\351ment dans plusieurs langues."));
	information_label -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	information_label -> setWordWrap(true);
	dialog_layout -> addWidget(information_label);
	
	// ajoute un NamesListWidget au dialogue
	NamesListWidget *names_widget = new NamesListWidget();
	names_widget -> setNames(_names);
	dialog_layout -> addWidget(names_widget);
	
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),     names_widget, SLOT(check()));
	connect(names_widget,   SIGNAL(inputChecked()), &dialog,      SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),     &dialog,      SLOT(reject()));
	
	// lance le dialogue
	if (dialog.exec() == QDialog::Accepted) {
		NamesList new_names(names_widget -> names());
		if (new_names != _names) undoStack().push(new ChangeNamesCommand(this, _names, new_names));
	}
}

/**
	Amene les elements selectionnes au premier plan
*/
void ElementScene::slot_bringForward() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::BringForward));
	emit(partsZValueChanged());
}

/**
	Remonte les elements selectionnes d'un plan
*/
void ElementScene::slot_raise() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::Raise));
	emit(partsZValueChanged());
}

/**
	Descend les elements selectionnes d'un plan
*/
void ElementScene::slot_lower() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::Lower));
	emit(partsZValueChanged());
}

/**
	Envoie les elements selectionnes au fond
*/
void ElementScene::slot_sendBackward() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::SendBackward));
	emit(partsZValueChanged());
}

/**
	@param include_terminals true pour inclure les bornes, false sinon
	@return les parties de l'element ordonnes par zValue croissante
*/
QList<QGraphicsItem *> ElementScene::zItems(bool include_terminals) const {
	// recupere les elements
	QList<QGraphicsItem *> all_items_list(items());
	
	// enleve les bornes
	QList<QGraphicsItem *> terminals;
	foreach(QGraphicsItem *qgi, all_items_list) {
		if (qgraphicsitem_cast<PartTerminal *>(qgi)) {
			all_items_list.removeAt(all_items_list.indexOf(qgi));
			terminals << qgi;
		}
	}
	
	// ordonne les parties par leur zValue
	QMultiMap<qreal, QGraphicsItem *> mm;
	foreach(QGraphicsItem *qgi, all_items_list) mm.insert(qgi -> zValue(), qgi);
	all_items_list.clear();
	QMapIterator<qreal, QGraphicsItem *> i(mm);
	while (i.hasNext()) {
		i.next();
		all_items_list << i.value();
	}
	
	// rajoute eventuellement les bornes
	if (include_terminals) all_items_list += terminals;
	return(all_items_list);
}

/**
	@return les parties graphiques selectionnees
*/
ElementContent ElementScene::selectedContent() const {
	ElementContent content;
	foreach(QGraphicsItem *qgi, zItems(true)) {
		if (qgi -> isSelected()) content << qgi;
	}
	return(content);
}

/**
	@param to_paste Rectangle englobant les parties a coller
	@return le rectangle ou il faudra coller ces parties
*/
void ElementScene::getPasteArea(const QRectF &to_paste) {
	// on le dessine sur la scene
	paste_area_ -> setRect(to_paste);
	addItem(paste_area_);
	
	// on passe la scene en mode "recherche de zone pour copier/coller"
	behavior = PasteArea;
}

/**
	Supprime les parties de l'element et les objets d'annulations.
	Les autres caracteristiques sont conservees.
*/
void ElementScene::reset() {
	// supprime les objets d'annulation
	undoStack().clear();
	// enleve les elements de la scene
	foreach (QGraphicsItem *qgi, items()) {
		removeItem(qgi);
		qgiManager().release(qgi);
	}
}

/**
	@param content Contenu ( = parties) d'un element
	@return le boundingRect de ces parties, exprime dans les coordonnes de la
	scene
*/
QRectF ElementScene::elementContentBoundingRect(const ElementContent &content) {
	QRectF bounding_rect;
	foreach(QGraphicsItem *qgi, content) {
		bounding_rect |= qgi -> sceneBoundingRect();
	}
	return(bounding_rect);
}

/**
	Applique les informations (dimensions, hostpot, orientations, connexions
	internes et noms) contenu dans un document XML.
	@param xml_document Document XML a analyser
	@param error_message pointeur vers une QString ; si error_message est
	different de 0, un message d'erreur sera stocke dedans si necessaire
	@return true si la lecture et l'application des informations s'est bien
	passee, false sinon.
*/
bool ElementScene::applyInformations(const QDomDocument &xml_document, QString *error_message) {
	// la racine est supposee etre une definition d'element
	QDomElement root = xml_document.documentElement();
	if (root.tagName() != "definition" || root.attribute("type") != "element") {
		if (error_message) {
			*error_message = tr("Ce document XML n'est pas une d\351finition d'\351l\351ment.", "error message");
		}
		return(false);
	}
	
	// dimensions et hotspot : ces attributs doivent etre presents et valides
	int w, h, hot_x, hot_y;
	if (
		!QET::attributeIsAnInteger(root, QString("width"),     &w) ||\
		!QET::attributeIsAnInteger(root, QString("height"),    &h) ||\
		!QET::attributeIsAnInteger(root, QString("hotspot_x"), &hot_x) ||\
		!QET::attributeIsAnInteger(root, QString("hotspot_y"), &hot_y)
	) {
		if (error_message) {
			*error_message = tr("Les dimensions ou le point de saisie ne sont pas valides.", "error message");
		}
		return(false);
	}
	//
	setWidth(w);
	setHeight(h);
	setHotspot(QPoint(hot_x, hot_y));
	
	// orientations
	internal_connections = (root.attribute("ic") == "true");
	
	// connexions internes
	if (!ori.fromString(root.attribute("orientation"))) {
		if (error_message) {
			*error_message = tr("Les orientations ne sont pas valides.", "error message");
		}
		return(false);
	}
	
	// extrait les noms de la definition XML
	_names.fromXml(root);
	
	return(true);
}

/**
	Par le document XML xml_document et retourne le contenu ( = liste de
	parties) correspondant.
	@param xml_document Document XML a analyser
	@param error_message pointeur vers une QString ; si error_message est
	different de 0, un message d'erreur sera stocke dedans si necessaire
*/
ElementContent ElementScene::loadContent(const QDomDocument &xml_document, QString *error_message) {
	ElementContent loaded_parts;
	
	// la racine est supposee etre une definition d'element
	QDomElement root = xml_document.documentElement();
	if (root.tagName() != "definition" || root.attribute("type") != "element") {
		if (error_message) {
			*error_message = tr("Ce document XML n'est pas une d\351finition d'\351l\351ment.", "error message");
		}
		return(loaded_parts);
	}
	
	// chargement de la description graphique de l'element
	for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		QDomElement elmts = node.toElement();
		if (elmts.isNull()) continue;
		if (elmts.tagName() == "description") {
			
			//  = parcours des differentes parties du dessin
			int z = 1;
			for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
				QDomElement qde = n.toElement();
				if (qde.isNull()) continue;
				CustomElementPart *cep;
				if      (qde.tagName() == "line")     cep = new PartLine     (element_editor, 0, 0);
				else if (qde.tagName() == "rect")     cep = new PartRectangle(element_editor, 0, 0);
				else if (qde.tagName() == "ellipse")  cep = new PartEllipse  (element_editor, 0, 0);
				else if (qde.tagName() == "circle")   cep = new PartCircle   (element_editor, 0, 0);
				else if (qde.tagName() == "polygon")  cep = new PartPolygon  (element_editor, 0, 0);
				else if (qde.tagName() == "terminal") cep = new PartTerminal (element_editor, 0, 0);
				else if (qde.tagName() == "text")     cep = new PartText     (element_editor, 0, 0);
				else if (qde.tagName() == "input")    cep = new PartTextField(element_editor, 0, 0);
				else if (qde.tagName() == "arc")      cep = new PartArc      (element_editor, 0, 0);
				else continue;
				if (QGraphicsItem *qgi = dynamic_cast<QGraphicsItem *>(cep)) {
					qgi -> setZValue(z++);
					loaded_parts << qgi;
				}
				cep -> fromXml(qde);
			}
		}
	}
	
	return(loaded_parts);
}

/**
	Ajoute le contenu content a cet element
	@param content contenu ( = liste de parties) a charger
	@param error_message pointeur vers une QString ; si error_message est
	different de 0, un message d'erreur sera stocke dedans si necessaire
	@return Le contenu ajoute
*/
ElementContent ElementScene::addContent(const ElementContent &content, QString */*error_message*/) {
	foreach(QGraphicsItem *part, content) {
		addItem(part);
	}
	return(content);
}

/**
	Ajoute le contenu content a cet element
	@param content contenu ( = liste de parties) a charger
	@param pos Position du coin superieur gauche du contenu apres avoir ete ajoute
	@param error_message pointeur vers une QString ; si error_message est
	different de 0, un message d'erreur sera stocke dedans si necessaire
	@return Le contenu ajoute
*/
ElementContent ElementScene::addContentAtPos(const ElementContent &content, const QPointF &pos, QString */*error_message*/) {
	// calcule le boundingRect du contenu a ajouter
	QRectF bounding_rect = elementContentBoundingRect(content);
	
	// en deduit le decalage a appliquer aux parties pour les poser au point demander
	QPointF offset = pos - bounding_rect.topLeft();
	
	// ajoute les parties avec le decalage adequat
	foreach(QGraphicsItem *part, content) {
		part -> setPos(part -> pos() + offset);
		addItem(part);
	}
	return(content);
}

/**
	Initialise la zone de collage
*/
void ElementScene::initPasteArea() {
	paste_area_ = new QGraphicsRectItem();
	paste_area_ -> setZValue(1000000);
	
	QPen paste_area_pen;
	paste_area_pen.setStyle(Qt::DashDotLine);
	paste_area_pen.setColor(QColor(30, 56, 86, 255));
	
	QBrush paste_area_brush;
	paste_area_brush.setStyle(Qt::SolidPattern);
	paste_area_brush.setColor(QColor(90, 167, 255, 64));
	
	paste_area_ -> setPen(paste_area_pen);
	paste_area_ -> setBrush(paste_area_brush);
}

/**
	Arrondit les coordonnees du point passees en parametre de facon a ce que ce
	point soit aligne sur la grille.
	@param point une reference vers un QPointF. Cet objet sera modifie.
	
*/
void ElementScene::snapToGrid(QPointF &point) {
	point.rx() = qRound(point.x() / x_grid) * x_grid;
	point.ry() = qRound(point.y() / y_grid) * y_grid;
}

/**
	@param e Evenement souris
	@return true s'il faut utiliser le snap-to-grid
	Typiquement, cette methode retourne true si l'evenement souris se produit
	sans la touche Ctrl enfoncee.
*/
bool ElementScene::mustSnapToGrid(QGraphicsSceneMouseEvent *e) {
	return(!(e -> modifiers() & Qt::ControlModifier));
}
