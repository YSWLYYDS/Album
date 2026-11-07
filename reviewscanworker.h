#pragma once
#include "imageprobe.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QObject>
#include <QSet>
#include <atomic>
#include <memory>
class ReviewScanWorker : public QObject {
  Q_OBJECT
public:
  ReviewScanWorker(QString root, std::shared_ptr<std::atomic_bool> cancel,
                   QStringList inputs = {})
      : m_root(std::move(root)), m_inputs(std::move(inputs)),
        m_cancel(std::move(cancel)) {}
public slots:
  void run() {
    QList<QVariantMap> chunk;
    QSet<QString> seen;
    int scanned = 0, accepted = 0, rejected = 0;
    auto visit = [&](const QString &path) {
      if (m_cancel->load())
        return;
      const auto suffix = QFileInfo(path).suffix().toLower();
      if (!QStringList{"png", "jpg", "jpeg", "bmp", "webp"}.contains(suffix)) {
        ++rejected;
        emit issue(path, "非支持的图片扩展名，已跳过");
        return;
      }
      const auto key = QFileInfo(path).canonicalFilePath();
      if (seen.contains(key))
        return;
      seen.insert(key);
      ++scanned;
      QString reason;
      auto metadata = ImageProbe::inspect(path, &reason);
      if (metadata.isEmpty()) {
        ++rejected;
        emit issue(path, reason);
      } else {
        ++accepted;
        chunk << metadata;
      }
      if (chunk.size() >= 200) {
        emit batch(chunk);
        chunk.clear();
      }
      if (scanned % 50 == 0)
        emit progress(scanned, accepted, rejected);
    };
    const QStringList sources =
        m_inputs.isEmpty() ? QStringList{m_root} : m_inputs;
    for (const auto &source : sources) {
      if (m_cancel->load())
        break;
      if (QFileInfo(source).isDir()) {
        QDirIterator it(source, QDir::Files | QDir::NoSymLinks,
                        QDirIterator::Subdirectories);
        while (it.hasNext() && !m_cancel->load())
          visit(it.next());
      } else
        visit(source);
    }
    if (!chunk.isEmpty())
      emit batch(chunk);
    emit progress(scanned, accepted, rejected);
    emit finished(m_cancel->load());
  }
signals:
  void batch(const QList<QVariantMap> &images);
  void issue(const QString &path, const QString &reason);
  void progress(int scanned, int accepted, int rejected);
  void finished(bool cancelled);

private:
  QString m_root;
  QStringList m_inputs;
  std::shared_ptr<std::atomic_bool> m_cancel;
};
