#include "reviewrepository.h"
#include "imageprobe.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSaveFile>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringConverter>
#include <QTextStream>
#include <QUuid>
#include <algorithm>
namespace {
QString now() {
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
QVariantMap row(const QSqlQuery &q) {
  QVariantMap m;
  for (int i = 0; i < q.record().count(); ++i)
    m[q.record().fieldName(i)] = q.value(i);
  return m;
}
QString csv(QString s) {
  if (!s.isEmpty() && QString("=+-@\t\r\n").contains(s.front()))
    s.prepend('\'');
  s.replace('"', "\"\"");
  return '"' + s + '"';
}
} // namespace
ReviewRepository::ReviewRepository(const QString &path)
    : m_path(path),
      m_name("review_" + QUuid::createUuid().toString(QUuid::WithoutBraces)) {}
ReviewRepository::~ReviewRepository() {
  if (m_db.isValid()) {
    m_db.close();
    m_db = {};
    QSqlDatabase::removeDatabase(m_name);
  }
}
bool ReviewRepository::open() {
  m_error.clear();
  QDir().mkpath(QFileInfo(m_path).absolutePath());
  m_db = QSqlDatabase::addDatabase("QSQLITE", m_name);
  m_db.setDatabaseName(m_path);
  m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
  if (!m_db.open()) {
    m_error = m_db.lastError().text();
    return false;
  }
  const QStringList sql{
      "PRAGMA foreign_keys=ON",
      "CREATE TABLE IF NOT EXISTS review_batches(id INTEGER PRIMARY KEY "
      "AUTOINCREMENT,product TEXT NOT NULL,number TEXT NOT NULL,root TEXT NOT "
      "NULL,created_at TEXT NOT NULL,UNIQUE(product,number))",
      "CREATE TABLE IF NOT EXISTS review_images(id INTEGER PRIMARY KEY "
      "AUTOINCREMENT,batch_id INTEGER NOT NULL REFERENCES "
      "review_batches(id),path TEXT NOT NULL,status TEXT NOT NULL DEFAULT "
      "'pending' CHECK(status IN ('pending','pass','fail')),defect TEXT NOT "
      "NULL DEFAULT '',note TEXT NOT NULL DEFAULT '',reviewer TEXT NOT NULL "
      "DEFAULT '',reviewed_at TEXT NOT NULL DEFAULT '',imported_at TEXT NOT "
      "NULL,version INTEGER NOT NULL DEFAULT 0,UNIQUE(batch_id,path))",
      "CREATE TABLE IF NOT EXISTS review_events(id INTEGER PRIMARY KEY "
      "AUTOINCREMENT,image_id INTEGER NOT NULL REFERENCES "
      "review_images(id),old_value TEXT NOT NULL,new_value TEXT NOT "
      "NULL,reviewer TEXT NOT NULL,created_at TEXT NOT NULL)",
      "CREATE INDEX IF NOT EXISTS idx_review_batch_status ON "
      "review_images(batch_id,status,id)",
      "CREATE INDEX IF NOT EXISTS idx_review_history ON "
      "review_events(image_id,id)"};
  for (const auto &s : sql) {
    QSqlQuery q(m_db);
    if (!q.exec(s)) {
      m_error = q.lastError().text();
      return false;
    }
  }
  QSqlQuery columns(m_db);
  bool hasChannel = false;
  if (!columns.exec("PRAGMA table_info(review_batches)")) {
    m_error = columns.lastError().text();
    return false;
  }
  while (columns.next())
    hasChannel |= columns.value(1).toString() == "channel";
  columns.finish();
  if (!hasChannel && !columns.exec("ALTER TABLE review_batches ADD COLUMN "
                                   "channel TEXT NOT NULL DEFAULT ''")) {
    m_error = columns.lastError().text();
    return false;
  }
  QSet<QString> imageColumns;
  if (!columns.exec("PRAGMA table_info(review_images)")) {
    m_error = columns.lastError().text();
    return false;
  }
  while (columns.next())
    imageColumns.insert(columns.value(1).toString());
  columns.finish();
  const QMap<QString, QString> additions{
      {"width", "INTEGER NOT NULL DEFAULT 0"},
      {"height", "INTEGER NOT NULL DEFAULT 0"},
      {"bytes", "INTEGER NOT NULL DEFAULT 0"},
      {"format", "TEXT NOT NULL DEFAULT ''"},
      {"modified_at", "TEXT NOT NULL DEFAULT ''"},
      {"tags", "TEXT NOT NULL DEFAULT ''"}};
  for (auto it = additions.cbegin(); it != additions.cend(); ++it)
    if (!imageColumns.contains(it.key()) &&
        !columns.exec("ALTER TABLE review_images ADD COLUMN " + it.key() + " " +
                      it.value())) {
      m_error = columns.lastError().text();
      return false;
    }
  return true;
}
qint64 ReviewRepository::createBatch(const QString &product,
                                     const QString &number, const QString &root,
                                     const QString &channel) {
  m_error.clear();
  if (product.trimmed().isEmpty() || number.trimmed().isEmpty() ||
      product.size() > 100 || number.size() > 100 || channel.size() > 100 ||
      !QFileInfo(root).isDir()) {
    m_error = "投放项目、素材批次及有效目录必填（名称不超过100字）";
    return -1;
  }
  QSqlQuery q(m_db);
  q.prepare("SELECT id,root,channel FROM review_batches WHERE product=? AND "
            "number=?");
  q.addBindValue(product.trimmed());
  q.addBindValue(number.trimmed());
  if (!q.exec()) {
    m_error = q.lastError().text();
    return -1;
  }
  const QString path = QFileInfo(root).canonicalFilePath();
  if (q.next()) {
    if (q.value(1).toString() != path ||
        q.value(2).toString() != channel.trimmed()) {
      m_error =
          "相同投放项目与素材批次已绑定其他目录或渠道，请使用不同批次编号";
      return -1;
    }
    return q.value(0).toLongLong();
  }
  q.prepare(
      "INSERT INTO review_batches(product,number,root,created_at,channel) "
      "VALUES(?,?,?,?,?)");
  for (const auto &s :
       QStringList{product.trimmed(), number.trimmed(), path, now(),
                   channel.isNull() ? QStringLiteral("") : channel.trimmed()})
    q.addBindValue(s);
  if (!q.exec()) {
    m_error = q.lastError().text();
    return -1;
  }
  return q.lastInsertId().toLongLong();
}
QList<QVariantMap> ReviewRepository::batches() {
  m_error.clear();
  QSqlQuery q(m_db);
  QList<QVariantMap> r;
  if (!q.exec("SELECT * FROM review_batches ORDER BY id DESC")) {
    m_error = q.lastError().text();
    return r;
  }
  while (q.next())
    r.append(row(q));
  return r;
}
bool ReviewRepository::addImages(qint64 batch, const QStringList &paths,
                                 int *inserted) {
  QList<QVariantMap> valid;
  for (const auto &p : paths) {
    QString error;
    auto meta = ImageProbe::inspect(p, &error);
    if (!meta.isEmpty())
      valid << meta;
  }
  return addValidatedImages(batch, valid, inserted);
}
bool ReviewRepository::addValidatedImages(qint64 batch,
                                          const QList<QVariantMap> &images,
                                          int *inserted) {
  m_error.clear();
  if (inserted)
    *inserted = 0;
  if (!m_db.transaction()) {
    m_error = m_db.lastError().text();
    return false;
  }
  QSqlQuery q(m_db);
  q.prepare("INSERT INTO "
            "review_images(batch_id,path,imported_at,width,height,bytes,format,"
            "modified_at) "
            "VALUES(?,?,?,?,?,?,?,?) ON CONFLICT(batch_id,path) DO NOTHING");
  int n = 0;
  for (const auto &meta : images) {
    QFileInfo f(meta.value("path").toString());
    if (!f.isFile() || f.isSymLink())
      continue;
    const QString path = f.canonicalFilePath();
    if (path.isEmpty())
      continue;
    q.bindValue(0, batch);
    q.bindValue(1, path);
    q.bindValue(2, now());
    int col = 3;
    for (const auto &name :
         QStringList{"width", "height", "bytes", "format", "modified_at"})
      q.bindValue(col++, meta.value(name));
    if (!q.exec()) {
      m_error = q.lastError().text();
      m_db.rollback();
      return false;
    }
    n += q.numRowsAffected();
  }
  if (!m_db.commit()) {
    m_error = m_db.lastError().text();
    m_db.rollback();
    return false;
  }
  if (inserted)
    *inserted = n;
  return true;
}
QString ReviewRepository::where() const {
  return " WHERE i.batch_id=? AND (?='' OR i.status=?) AND (i.path LIKE ? "
         "ESCAPE '\\' OR i.defect LIKE ? ESCAPE '\\' OR i.note LIKE ? ESCAPE "
         "'\\' OR i.tags LIKE ? ESCAPE '\\')";
}
void ReviewRepository::bindFilter(QSqlQuery &q, qint64 batch,
                                  const QString &status,
                                  const QString &keyword) {
  QString key = keyword;
  key.replace("\\", "\\\\");
  key.replace("%", "\\%");
  key.replace("_", "\\_");
  q.addBindValue(batch);
  q.addBindValue(status);
  q.addBindValue(status);
  for (int i = 0; i < 4; ++i)
    q.addBindValue("%" + key + "%");
}
QList<QVariantMap> ReviewRepository::images(qint64 batch, const QString &status,
                                            const QString &key, int offset,
                                            int limit) {
  m_error.clear();
  QSqlQuery q(m_db);
  q.prepare("SELECT i.* FROM review_images i" + where() +
            " ORDER BY i.id LIMIT ? OFFSET ?");
  bindFilter(q, batch, status, key);
  q.addBindValue(std::clamp(limit, 1, 100));
  q.addBindValue(qMax(0, offset));
  QList<QVariantMap> r;
  if (!q.exec()) {
    m_error = q.lastError().text();
    return r;
  }
  while (q.next())
    r.append(row(q));
  return r;
}
int ReviewRepository::count(qint64 batch, const QString &status,
                            const QString &key) {
  m_error.clear();
  QSqlQuery q(m_db);
  q.prepare("SELECT COUNT(*) FROM review_images i" + where());
  bindFilter(q, batch, status, key);
  if (!q.exec() || !q.next()) {
    m_error = q.lastError().text();
    return -1;
  }
  return q.value(0).toInt();
}
bool ReviewRepository::review(qint64 image, const QString &status,
                              const QString &defect, const QString &note,
                              const QString &tags, const QString &reviewer,
                              int version) {
  return reviewMany({QVariantMap{{"id", image}, {"version", version},
                                 {"tags", tags}}},
                    status, defect, note, reviewer);
}
bool ReviewRepository::reviewMany(const QList<QVariantMap> &images,
                                  const QString &status, const QString &defect,
                                  const QString &note,
                                  const QString &reviewer) {
  m_error.clear();
  if (images.isEmpty() || images.size() > 100) {
    m_error = "请选择1至100条素材";
    return false;
  }
  if (!m_db.transaction()) {
    m_error = m_db.lastError().text();
    return false;
  }
  for (const auto &image : images)
    if (!applyReview(image.value("id").toLongLong(), status, defect, note,
                     image.value("tags").toString(), reviewer,
                     image.value("version").toInt())) {
      m_db.rollback();
      return false;
    }
  if (!m_db.commit()) {
    m_error = m_db.lastError().text();
    m_db.rollback();
    return false;
  }
  return true;
}
QVariantMap ReviewRepository::statistics(qint64 batch) {
  m_error.clear();
  QSqlQuery q(m_db);
  q.prepare("SELECT COUNT(*) AS total,SUM(status='pending') AS "
            "pending,SUM(status='pass') AS pass,SUM(status='fail') AS fail "
            "FROM review_images WHERE batch_id=?");
  q.addBindValue(batch);
  if (!q.exec() || !q.next()) {
    m_error = q.lastError().text();
    return {};
  }
  return row(q);
}
bool ReviewRepository::applyReview(qint64 image, const QString &status,
                                   const QString &defect, const QString &note,
                                   const QString &tags, const QString &reviewer,
                                   int version) {
  m_error.clear();
  if (!QStringList{"pending", "pass", "fail"}.contains(status) ||
      reviewer.trimmed().isEmpty() || reviewer.size() > 64 ||
      note.size() > 2000 || defect.size() > 100 || tags.size() > 200 ||
      (status == "fail" && defect.trimmed().isEmpty())) {
    m_error = "审核人必填，退回需填写退回原因，标签和字段长度不得超限";
    return false;
  }
  QSqlQuery q(m_db);
  q.prepare("SELECT * FROM review_images WHERE id=?");
  q.addBindValue(image);
  if (!q.exec() || !q.next()) {
    m_error = "图片记录不存在";
    return false;
  }
  const auto before = row(q);
  const QFileInfo source(before.value("path").toString());
  if (before.value("bytes").toLongLong() > 0 &&
      (source.size() != before.value("bytes").toLongLong() ||
       source.lastModified().toUTC().toString(Qt::ISODateWithMs) !=
           before.value("modified_at").toString())) {
    m_error = "源图片在导入后发生变化，请重新登记批次，避免审核错版素材";
    return false;
  }
  QImageReader reader(before.value("path").toString());
  if (!reader.canRead()) {
    m_error = "原图不存在或格式损坏，不能提交判定";
    return false;
  }
  const QString time = now(),
                kind = status == "fail" ? defect.trimmed() : QStringLiteral("");
  QSqlQuery update(m_db);
  update.prepare("UPDATE review_images SET "
                 "status=?,defect=?,note=?,tags=?,reviewer=?,reviewed_at=?,version="
                 "version+1 WHERE id=? AND version=?");
  update.addBindValue(status);
  update.addBindValue(kind);
  update.addBindValue(note.trimmed().isEmpty() ? QStringLiteral("")
                                               : note.trimmed());
  update.addBindValue(tags.trimmed());
  update.addBindValue(reviewer.trimmed());
  update.addBindValue(time);
  update.addBindValue(image);
  update.addBindValue(version);
  if (!update.exec() || update.numRowsAffected() != 1) {
    m_error = "保存失败或记录已被修改，请刷新后重试";
    return false;
  }
  auto after = before;
  after["status"] = status;
  after["defect"] = kind;
  after["note"] = note.trimmed();
  after["tags"] = tags.trimmed();
  after["reviewer"] = reviewer.trimmed();
  after["reviewed_at"] = time;
  after["version"] = version + 1;
  QSqlQuery audit(m_db);
  audit.prepare("INSERT INTO "
                "review_events(image_id,old_value,new_value,reviewer,created_"
                "at) VALUES(?,?,?,?,?)");
  audit.addBindValue(image);
  audit.addBindValue(
      QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(before))
                            .toJson(QJsonDocument::Compact)));
  audit.addBindValue(
      QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(after))
                            .toJson(QJsonDocument::Compact)));
  audit.addBindValue(reviewer.trimmed());
  audit.addBindValue(time);
  if (!audit.exec()) {
    m_error = audit.lastError().text();
    return false;
  }
  return true;
}
QList<QVariantMap> ReviewRepository::history(qint64 image) {
  m_error.clear();
  QSqlQuery q(m_db);
  q.prepare("SELECT * FROM review_events WHERE image_id=? ORDER BY id DESC");
  q.addBindValue(image);
  QList<QVariantMap> r;
  if (!q.exec()) {
    m_error = q.lastError().text();
    return r;
  }
  while (q.next())
    r.append(row(q));
  return r;
}
bool ReviewRepository::exportCsv(const QString &path, qint64 batch,
                                 const QString &status, const QString &key) {
  m_error.clear();
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    m_error = file.errorString();
    return false;
  }
  file.write("\xEF\xBB\xBF");
  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);
  out << "投放项目,渠道,素材批次,素材路径,文件存在,审核状态,退回原因,素材标签,备注,"
         "审核人,审核时间,宽度,高度,字节数,格式,源文件修改时间\r\n";
  QSqlQuery q(m_db);
  q.prepare("SELECT b.product,b.channel,b.number,i.* FROM review_images i JOIN "
            "review_batches b ON b.id=i.batch_id" +
            where() + " ORDER BY i.id");
  bindFilter(q, batch, status, key);
  if (!q.exec()) {
    m_error = q.lastError().text();
    return false;
  }
  while (q.next()) {
    QStringList values;
    for (const auto &name : QStringList{"product", "channel", "number", "path"})
      values << csv(q.value(name).toString());
    values << csv(QFileInfo::exists(q.value("path").toString()) ? "是" : "否");
    for (const auto &name :
         QStringList{"status", "defect", "tags", "note", "reviewer", "reviewed_at",
                     "width", "height", "bytes", "format", "modified_at"})
      values << csv(q.value(name).toString());
    out << values.join(',') << "\r\n";
  }
  out.flush();
  if (out.status() != QTextStream::Ok) {
    m_error = "CSV写入失败";
    return false;
  }
  if (!file.commit()) {
    m_error = file.errorString();
    return false;
  }
  return true;
}
