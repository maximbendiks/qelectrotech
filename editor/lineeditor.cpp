/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "lineeditor.h"
#include "partline.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param line La ligne a editer
	@param parent le Widget parent
*/
LineEditor::LineEditor(QETElementEditor *editor, PartLine *line, QWidget *parent) : ElementItemEditor(editor, parent) {
	
	part = line;
	
	x1 = new QLineEdit();
	y1 = new QLineEdit();
	x2 = new QLineEdit();
	y2 = new QLineEdit();
	
	x1 -> setValidator(new QDoubleValidator(x1));
	y1 -> setValidator(new QDoubleValidator(y1));
	x2 -> setValidator(new QDoubleValidator(x2));
	y2 -> setValidator(new QDoubleValidator(y2));
	
	QGridLayout *grid = new QGridLayout(this);
	grid -> addWidget(new QLabel("x1"), 0, 0);
	grid -> addWidget(x1,               0, 1);
	grid -> addWidget(new QLabel("y1"), 0, 2);
	grid -> addWidget(y1,               0, 3);
	grid -> addWidget(new QLabel("x2"), 1, 0);
	grid -> addWidget(x2,               1, 1);
	grid -> addWidget(new QLabel("y2"), 1, 2);
	grid -> addWidget(y2,               1, 3);
	
	updateForm();
}

/// Destructeur
LineEditor::~LineEditor() {
}

/**
	Met a jour la ligne a partir des donnees du formulaire
*/
void LineEditor::updateLine() {
	part -> setLine(
		QLineF(
			part -> mapFromScene(
				x1 -> text().toDouble(),
				y1 -> text().toDouble()
			),
			part -> mapFromScene(
				x2 -> text().toDouble(),
				y2 -> text().toDouble()
			)
		)
	);
}

/// Met a jour l'abscisse du premier point de la ligne et cree un objet d'annulation
void LineEditor::updateLineX1() { addChangePartCommand(tr("abscisse point 1"),    part, "x1", x1 -> text().toDouble()); }
/// Met a jour l'ordonnee du premier point de la ligne et cree un objet d'annulation
void LineEditor::updateLineY1() { addChangePartCommand(tr("ordonn\351e point 1"), part, "y1", y1 -> text().toDouble()); }
/// Met a jour l'abscisse du second point de la ligne et cree un objet d'annulation
void LineEditor::updateLineX2() { addChangePartCommand(tr("abscisse point 2"),    part, "x2", x2 -> text().toDouble()); }
/// Met a jour l'ordonnee du second point de la ligne et cree un objet d'annulation
void LineEditor::updateLineY2() { addChangePartCommand(tr("ordonn\351e point 2"), part, "y2", y2 -> text().toDouble()); }

/**
	Met a jour le formulaire d'edition
*/
void LineEditor::updateForm() {
	activeConnections(false);
	QPointF p1(part -> sceneP1());
	QPointF p2(part -> sceneP2());
	x1 -> setText(QString("%1").arg(p1.x()));
	y1 -> setText(QString("%1").arg(p1.y()));
	x2 -> setText(QString("%1").arg(p2.x()));
	y2 -> setText(QString("%1").arg(p2.y()));
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void LineEditor::activeConnections(bool active) {
	if (active) {
		connect(x1, SIGNAL(editingFinished()), this, SLOT(updateLineX1()));
		connect(y1, SIGNAL(editingFinished()), this, SLOT(updateLineY1()));
		connect(x2, SIGNAL(editingFinished()), this, SLOT(updateLineX2()));
		connect(y2, SIGNAL(editingFinished()), this, SLOT(updateLineY2()));
	} else {
		connect(x1, SIGNAL(editingFinished()), this, SLOT(updateLineX1()));
		connect(y1, SIGNAL(editingFinished()), this, SLOT(updateLineY1()));
		connect(x2, SIGNAL(editingFinished()), this, SLOT(updateLineX2()));
		connect(y2, SIGNAL(editingFinished()), this, SLOT(updateLineY2()));
	}
}
