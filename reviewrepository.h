#pragma once
#include <QSqlDatabase>
#include <QStringList>
#include <QVariantMap>

class ReviewRepository {
public:
  explicit ReviewRepository(const QString &path);
  ~ReviewRepository();
  bool open();
  qint64 createBatch(const QString &product, const QString &number,
                     const QString &root, const QString &channel = QString());
  QList<QVariantMap> batches();
  bool addImages(qint64 batch, const QStringList &paths,
                 int *inserted = nullptr);
  bool addValidatedImages(qint64 batch, const QList<QVariantMap> &images,
                          int *inserted = nullptr);
  QVariantMap statistics(qint64 batch);
  bool reviewMany(const QList<QVariantMap> &images, const QString &status,
                  const QString &defect, const QString &note,
                  const QString &reviewer);
  QList<QVariantMap> images(qint64 batch, const QString &status,
                            const QString &keyword, int offset,
                            int limit = 100);
  int count(qint64 batch, const QString &status, const QString &keyword);
  bool review(qint64 image, const QString &status, const QString &defect,
              const QString &note, const QString &tags,
              const QString &reviewer, int version);
  QList<QVariantMap> history(qint64 image);
  bool exportCsv(const QString &path, qint64 batch, const QString &status,
                 const QString &keyword);
  QString lastError() const { return m_error; }

private:
  bool applyReview(qint64 image, const QString &status, const QString &defect,
                   const QString &note, const QString &tags,
                   const QString &reviewer, int version);
  QString m_path, m_name, m_error;
  QSqlDatabase m_db;
  QString where() const;
  void bindFilter(class QSqlQuery &q, qint64 batch, const QString &status,
                  const QString &keyword);
};
