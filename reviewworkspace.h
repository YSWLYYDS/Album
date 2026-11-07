#pragma once
#include "reviewrepository.h"
#include <QImage>
#include <QVariantMap>
#include <QWidget>
#include <atomic>
#include <memory>
class QComboBox;
class QLineEdit;
class QTableWidget;
class QLabel;
class QTextEdit;
class QPushButton;
class QThread;
class QProgressBar;
class QCheckBox;
class FitImageView;
class ReviewWorkspace : public QWidget {
  Q_OBJECT
public:
  explicit ReviewWorkspace(QWidget *parent = nullptr);
  ~ReviewWorkspace() override;

private:
  void beginImport(const QStringList &sources, bool append = false);
  void batchReview();
  void showImportLog();
  void enlargePreview();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  void loadBatches(qint64 select = -1);
  void refresh();
  void selected();
  void importDirectory();
  void saveReview();
  void history();
  void previewReady(int generation, const QString &path, const QImage &image);
  qint64 batchId() const;
  QString statusFilter() const;
  void error(const QString &message);
  ReviewRepository m_repo;
  QComboBox *m_batches, *m_filter, *m_verdict;
  QLineEdit *m_key, *m_reviewer, *m_defect, *m_tags;
  QTextEdit *m_note;
  QTableWidget *m_table;
  FitImageView *m_preview;
  QLabel *m_info, *m_path;
  QCheckBox *m_continue;
  QPushButton *m_import, *m_cancelButton, *m_save, *m_previous, *m_next;
  QThread *m_thread = nullptr;
  std::shared_ptr<std::atomic_bool> m_cancel;
  QList<QVariantMap> m_rows;
  QVariantMap m_selected;
  int m_offset = 0, m_generation = 0, m_imported = 0;
  QString m_currentPath;
  QString m_importError;
  QStringList m_importIssues;
  int m_rejected = 0, m_accepted = 0;
  QList<QLabel *> m_metrics;
  QLabel *m_metadata;
  QProgressBar *m_progress;
};
