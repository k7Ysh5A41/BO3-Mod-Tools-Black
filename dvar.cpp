#include "stdafx.h"

QString Dvar::SettingKey(const dvar_s& Dvar)
{
	return QString("dvar_%1").arg(Dvar.name);
}

QVariant Dvar::DefaultValue(const dvar_s& Dvar)
{
	switch (Dvar.type)
	{
	case DVAR_VALUE_BOOL:
		return false;
	case DVAR_VALUE_INT:
		return 0;
	case DVAR_VALUE_STRING:
	default:
		return QString();
	}
}

QVariant Dvar::StoredValue(const dvar_s& Dvar)
{
	QSettings Settings;
	return Settings.value(SettingKey(Dvar), DefaultValue(Dvar));
}

void Dvar::UpdateItemState(QTreeWidgetItem* Item, const dvar_s& Dvar, const QVariant& CurrentValue)
{
	if (!Item)
		return;

	const bool IsModified = (CurrentValue != DefaultValue(Dvar));
	Item->setText(0, IsModified ? QString("* %1").arg(Dvar.name) : QString(Dvar.name));
	Item->setToolTip(0, QString("%1\n\nDefault: %2").arg(Dvar.description, DefaultValue(Dvar).toString().isEmpty() ? QString("\"\"") : DefaultValue(Dvar).toString()));
}

Dvar::Dvar(dvar_s _dvar, QTreeWidget *_dvarTree) : dvar(_dvar)
{
	QTreeWidgetItem* Item = new QTreeWidgetItem(_dvarTree, QStringList() << dvar.name);
	Item->setText(0, dvar.name);
	Item->setData(0, Qt::UserRole, QString(dvar.name));

	QCheckBox* checkBox;
	QSpinBox* spinBox;
	QLineEdit* textBox;
	const QVariant SavedValue = StoredValue(dvar);
	UpdateItemState(Item, dvar, SavedValue);

	switch(this->dvar.type)
	{
	case DVAR_VALUE_BOOL:
		checkBox = new QCheckBox();
		checkBox->setChecked(SavedValue.toBool());
		checkBox->setToolTip("Boolean value, check to enable or uncheck to disable.");
		QObject::connect(checkBox, &QCheckBox::toggled, checkBox, [Item, _dvar](bool Checked)
		{
			UpdateItemState(Item, _dvar, Checked);
		});
		_dvarTree->setItemWidget(Item, 1, checkBox);
		break;
	case DVAR_VALUE_INT:
		spinBox = new QSpinBox();
		spinBox->setValue(SavedValue.toInt());
		spinBox->setToolTip("Integer value, min to max any number.");
		spinBox->setMaximum(dvar.maxValue);
		spinBox->setMinimum(dvar.minValue);
		QObject::connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), spinBox, [Item, _dvar](int Value)
		{
			UpdateItemState(Item, _dvar, Value);
		});
		_dvarTree->setItemWidget(Item, 1, spinBox);
		break;
	case DVAR_VALUE_STRING:
		textBox = new QLineEdit();
		textBox->setText(SavedValue.toString());
		textBox->setToolTip(QString("String value, leave this blank for it to not be used."));
		textBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		QObject::connect(textBox, &QLineEdit::textChanged, textBox, [Item, _dvar](const QString& Value)
		{
			UpdateItemState(Item, _dvar, Value);
		});
		_dvarTree->setItemWidget(Item, 1, textBox);
		break;
	}
}

Dvar::~Dvar()
{
}

dvar_s Dvar::findDvar(QString _dvarName, dvar_s* dvars, int DvarSize)
{
	dvar_s _dvar = {};
	for(int DvarIdx = 0; DvarIdx < DvarSize; DvarIdx++)
	{
		_dvar = dvars[DvarIdx];
		if(_dvar.name == _dvarName)
			return _dvar;
	}
	return _dvar;
}

QString Dvar::setDvarSetting(dvar_s _dvar, QCheckBox* _checkBox)
{
	QSettings Settings;
	if (_checkBox->isChecked() == DefaultValue(_dvar).toBool())
		Settings.remove(SettingKey(_dvar));
	else
		Settings.setValue(SettingKey(_dvar), _checkBox->isChecked());

	return _checkBox->isChecked() ? "1" : QString();
}

QString Dvar::setDvarSetting(dvar_s _dvar, QSpinBox* _spinBox)
{
	QSettings Settings;
	if (_spinBox->value() == DefaultValue(_dvar).toInt())
		Settings.remove(SettingKey(_dvar));
	else
		Settings.setValue(SettingKey(_dvar), _spinBox->value());

	return _spinBox->value() == DefaultValue(_dvar).toInt() ? QString() : QString::number(_spinBox->value());
}

QString Dvar::setDvarSetting(dvar_s _dvar, QLineEdit* _lineEdit)
{
	QSettings Settings;
	if (_lineEdit->text() == DefaultValue(_dvar).toString())
		Settings.remove(SettingKey(_dvar));
	else
		Settings.setValue(SettingKey(_dvar), _lineEdit->text());

	return _lineEdit->text();
}

QString Dvar::launchValue(dvar_s _dvar)
{
	const QVariant Value = StoredValue(_dvar);
	if (Value == DefaultValue(_dvar))
		return QString();

	switch (_dvar.type)
	{
	case DVAR_VALUE_BOOL:
		return Value.toBool() ? "1" : QString();
	case DVAR_VALUE_INT:
		return QString::number(Value.toInt());
	case DVAR_VALUE_STRING:
	default:
		return Value.toString();
	}
}
