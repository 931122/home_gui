#ifndef TSTRANSLATOR_H
#define TSTRANSLATOR_H

#include <QHash>
#include <QTranslator>

class TsTranslator : public QTranslator
{
    Q_OBJECT

public:
    explicit TsTranslator(QObject *parent = nullptr);

    bool loadFromTsFile(const QString &fileName);
    QString translate(const char *context,
                      const char *sourceText,
                      const char *disambiguation = nullptr,
                      int n = -1) const override;

private:
    QHash<QString, QHash<QString, QString>> m_messages;
};

#endif
