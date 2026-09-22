#include "tstranslator.h"

#include <QFile>
#include <QXmlStreamReader>

TsTranslator::TsTranslator(QObject *parent)
    : QTranslator(parent)
{
}

bool TsTranslator::loadFromTsFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QHash<QString, QHash<QString, QString>> messages;
    QXmlStreamReader reader(&file);
    QString contextName;

    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement()) {
            continue;
        }

        if (reader.name() == QStringLiteral("context")) {
            QString nextContextName;
            QHash<QString, QString> contextMessages;

            while (!(reader.isEndElement() && reader.name() == QStringLiteral("context")) && !reader.atEnd()) {
                reader.readNext();
                if (!reader.isStartElement()) {
                    continue;
                }

                if (reader.name() == QStringLiteral("name")) {
                    nextContextName = reader.readElementText();
                } else if (reader.name() == QStringLiteral("message")) {
                    QString source;
                    QString translation;
                    bool unfinished = false;

                    while (!(reader.isEndElement() && reader.name() == QStringLiteral("message")) && !reader.atEnd()) {
                        reader.readNext();
                        if (!reader.isStartElement()) {
                            continue;
                        }

                        if (reader.name() == QStringLiteral("source")) {
                            source = reader.readElementText();
                        } else if (reader.name() == QStringLiteral("translation")) {
                            unfinished = reader.attributes().value(QStringLiteral("type")) == QStringLiteral("unfinished");
                            translation = reader.readElementText();
                        }
                    }

                    if (!source.isEmpty() && !translation.isEmpty() && !unfinished) {
                        contextMessages.insert(source, translation);
                    }
                }
            }

            if (!nextContextName.isEmpty() && !contextMessages.isEmpty()) {
                messages.insert(nextContextName, contextMessages);
            }
        }
    }

    if (reader.hasError()) {
        return false;
    }

    m_messages = messages;
    return true;
}

QString TsTranslator::translate(const char *context,
                                const char *sourceText,
                                const char *disambiguation,
                                int n) const
{
    Q_UNUSED(disambiguation)
    Q_UNUSED(n)

    const auto contextIt = m_messages.constFind(QString::fromUtf8(context));
    if (contextIt == m_messages.constEnd()) {
        return QString();
    }

    return contextIt->value(QString::fromUtf8(sourceText));
}
