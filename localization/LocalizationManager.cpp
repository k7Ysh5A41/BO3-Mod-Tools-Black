#include "stdafx.h"
#include "localization/LocalizationManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>

LocalizationManager& LocalizationManager::Instance()
{
	static LocalizationManager instance;
	return instance;
}

void LocalizationManager::LoadAllLanguages(const QStringList& languages)
{
	for (const QString& lang : languages)
		LoadLanguage(lang);
}

bool LocalizationManager::LoadLanguage(const QString& language)
{
	QString lang = language.toLower();
	if (mTranslations.contains(lang) && !mTranslations[lang].isEmpty())
	{
		mCurrentLanguage = lang;
		return true;
	}

	QString filePath = mLocalizationPath + "/" + lang + ".json";
	if (!QFileInfo::exists(filePath))
	{
		if (lang == "english")
		{
			mCurrentLanguage = "english";
			return false;
		}
		return LoadLanguage("english");
	}

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (lang == "english")
		{
			mCurrentLanguage = "english";
			return false;
		}
		return LoadLanguage("english");
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (!doc.isObject())
	{
		if (lang == "english")
		{
			mCurrentLanguage = "english";
			return false;
		}
		return LoadLanguage("english");
	}

	QJsonObject root = doc.object();
	QJsonObject strings = root.value("strings").toObject();
	QHash<QString, QString> translationMap;
	for (auto it = strings.begin(); it != strings.end(); ++it)
		translationMap.insert(it.key(), it.value().toString());

	mTranslations[lang] = translationMap;

	if (!mLanguages.contains(lang))
		mLanguages.append(lang);

	mCurrentLanguage = lang;

	if (lang == "english")
		mEnglishTranslations = translationMap;

	return true;
}

QString LocalizationManager::tr(const QString& key) const
{
	if (mTranslations.contains(mCurrentLanguage))
	{
		auto it = mTranslations[mCurrentLanguage].find(key);
		if (it != mTranslations[mCurrentLanguage].end() && !it.value().isEmpty())
			return it.value();
	}

	if (mEnglishTranslations.contains(key))
		return mEnglishTranslations[key];

	return key;
}

QString LocalizationManager::tr(const QString& key, const QString& defaultValue) const
{
	if (mTranslations.contains(mCurrentLanguage))
	{
		auto it = mTranslations[mCurrentLanguage].find(key);
		if (it != mTranslations[mCurrentLanguage].end() && !it.value().isEmpty())
			return it.value();
	}

	if (mEnglishTranslations.contains(key))
		return mEnglishTranslations[key];

	return defaultValue;
}

QStringList LocalizationManager::availableLanguageDisplayNames() const
{
	QStringList displayNames;
	for (const QString& lang : mLanguages)
	{
		if (mTranslations.contains(lang))
		{
			QString nativeName = mTranslations[lang].value("language.native_name");
			if (nativeName.isEmpty())
				nativeName = lang;
			displayNames << nativeName;
		}
	}
	return displayNames;
}

bool LocalizationManager::SaveTranslation(const QString& language, const QString& key, const QString& value)
{
	QString lang = language.toLower();
	mTranslations[lang][key] = value;
	return true;
}

bool LocalizationManager::SaveTranslationFile(const QString& language)
{
	QString lang = language.toLower();
	QString filePath = mLocalizationPath + "/" + lang + ".json";

	QJsonObject root;

	QString nativeName = mTranslations[lang].value("language.native_name");
	root["language"] = lang;
	root["nativeName"] = nativeName;

	QJsonObject strings;
	if (mTranslations.contains(lang))
	{
		for (auto it = mTranslations[lang].constBegin(); it != mTranslations[lang].constEnd(); ++it)
			strings[it.key()] = it.value();
	}
	root["strings"] = strings;

	QJsonDocument doc(root);

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	file.write(doc.toJson(QJsonDocument::Indented));
	file.close();
	return true;
}

QJsonObject LocalizationManager::GetTranslations(const QString& language) const
{
	QString lang = language.toLower();
	QJsonObject result;
	if (mTranslations.contains(lang))
	{
		for (auto it = mTranslations[lang].constBegin(); it != mTranslations[lang].constEnd(); ++it)
			result[it.key()] = it.value();
	}
	return result;
}

QString LocalizationManager::GetEnglishReference(const QString& key) const
{
	if (mEnglishTranslations.contains(key))
		return mEnglishTranslations[key];
	return QString();
}
