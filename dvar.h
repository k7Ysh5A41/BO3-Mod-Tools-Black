#pragma once

enum DvarType
{
	DVAR_VALUE_BOOL,
	DVAR_VALUE_INT,
	DVAR_VALUE_STRING
};

struct dvar_s
{
	const char* name;
	const char* description;
	DvarType type;
	int minValue;
	int maxValue;
	bool isCmd;
};

class Dvar
{
private:
	dvar_s dvar;
	static QString SettingKey(const dvar_s&);
	static QVariant DefaultValue(const dvar_s&);
	static QVariant StoredValue(const dvar_s&);
	static void UpdateItemState(QTreeWidgetItem*, const dvar_s&, const QVariant&);

public:
	Dvar();
	Dvar(dvar_s, QTreeWidget*);
	~Dvar();

	static QString setDvarSetting(dvar_s, QCheckBox*);
	static QString setDvarSetting(dvar_s, QSpinBox*);
	static QString setDvarSetting(dvar_s, QLineEdit*);
	static QString launchValue(dvar_s);

	static dvar_s findDvar(QString, dvar_s*, int);
};
