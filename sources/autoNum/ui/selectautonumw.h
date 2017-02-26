/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef SELECTAUTONUMW_H
#define SELECTAUTONUMW_H

#include <QWidget>
#include "numerotationcontext.h"
#include "formulaautonumberingw.h"

class NumPartEditorW;
class QAbstractButton;
class FormulaAutonumberingW;

namespace Ui {
	class SelectAutonumW;
}

class SelectAutonumW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
		explicit SelectAutonumW(int type, QWidget *parent = 0);
		explicit SelectAutonumW(const NumerotationContext &context, int type, QWidget *parent = 0);
		~SelectAutonumW();

		void setContext (const NumerotationContext &context);
		NumerotationContext toNumContext() const;
		void contextToFormula ();
		QString formula();

	signals:
		void applyPressed();

	//SLOT
	public slots:
		void applyEnableOnContextChanged(QString);

	private slots:
		void on_add_button_clicked();
		void on_remove_button_clicked();
		void on_buttonBox_clicked(QAbstractButton *);
		void applyEnable (bool = true);

	//ATTRIBUTES
		void on_m_next_pb_clicked();
		void on_m_previous_pb_clicked();

	private:
		Ui::SelectAutonumW *ui;
		QList <NumPartEditorW *> num_part_list_;
		NumerotationContext m_context;
		FormulaAutonumberingW *m_feaw;
		FormulaAutonumberingW *m_fcaw;
		int m_edited_type = -1; //0 == element : 1 == conductor : 2 == folio
};

#endif // SELECTAUTONUMW_H
