#include "reviewworkspace.h"
#include "businessstyle.h"
#include "imagecanvas.h"
#include "fitimageview.h"
#include "reviewscanworker.h"
#include "thumbcache.h"
#include "thumbnailtask.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QtWidgets>
namespace {
QString label(const QString &s) {
  return QMap<QString, QString>{
      {"pending", "待审核"}, {"pass", "通过"}, {"fail", "退回"}}
      .value(s, s);
}
QString dbPath() {
  return qEnvironmentVariableIsSet("IMAGE_REVIEW_DATABASE")
             ? qEnvironmentVariable("IMAGE_REVIEW_DATABASE")
             : QStandardPaths::writableLocation(
                   QStandardPaths::AppLocalDataLocation) +
                   "/image-review.db";
}
} // namespace
ReviewWorkspace::ReviewWorkspace(QWidget *parent)
    : QWidget(parent), m_repo(dbPath()) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);
  setAcceptDrops(true);
  auto *metrics = new QHBoxLayout;
  for (const auto &caption :
       QStringList{"本批次素材", "待审核", "已通过", "已退回"}) {
    QLabel *value;
    metrics->addWidget(BusinessUi::metric(caption, value));
    m_metrics << value;
  }
  layout->addLayout(metrics);
  auto *bar = new QHBoxLayout;
  m_batches = new QComboBox;
  m_batches->setMinimumContentsLength(20);
  bar->addWidget(new QLabel("投放项目 / 渠道 / 素材批次"));
  bar->addWidget(m_batches, 1);
  m_import = new QPushButton("导入素材");
  m_import->setProperty("primary", true);
  auto *importMenu = new QMenu(m_import);
  auto *directoryAction = importMenu->addAction("新批次：导入目录");
  auto *filesAction = importMenu->addAction("新批次：选择多张图片");
  auto *appendAction = importMenu->addAction("追加图片到当前批次");
  m_import->setMenu(importMenu);
  m_import->setObjectName("importReviewBatch");
  m_cancelButton = new QPushButton("取消扫描");
  m_cancelButton->setEnabled(false);
  bar->addWidget(m_import);
  bar->addWidget(m_cancelButton);
  layout->addLayout(bar);
  m_progress = new QProgressBar;
  m_progress->setRange(0, 0);
  m_progress->setVisible(false);
  layout->addWidget(m_progress);
  auto *filters = new QHBoxLayout;
  m_filter = new QComboBox;
  m_filter->addItem("全部审核状态", "");
  for (const auto &s : QStringList{"pending", "pass", "fail"})
    m_filter->addItem(label(s), s);
  m_key = new QLineEdit;
  m_key->setPlaceholderText("搜索文件、标签、退回原因或备注");
  m_key->setMaxLength(100);
  auto *search = new QPushButton("查询 / 刷新");
  auto *exportButton = new QPushButton("导出当前筛选全部记录 CSV");
  auto *bulk = new QPushButton("批量审核");
  bulk->setEnabled(false);
  bulk->setObjectName("bulkReview");
  auto *log = new QPushButton("导入记录");
  filters->addWidget(m_filter);
  filters->addWidget(m_key, 1);
  filters->addWidget(search);
  filters->addWidget(exportButton);
  filters->addWidget(bulk);
  filters->addWidget(log);
  layout->addLayout(filters);
  auto *split = new QSplitter;
  m_table = new QTableWidget(0, 7);
  m_table->setObjectName("reviewImageTable");
  m_table->setHorizontalHeaderLabels(
      {"素材", "审核状态", "素材标签", "退回原因", "审核人", "时间", "文件"});
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_table->setAlternatingRowColors(true);
  m_table->verticalHeader()->setDefaultSectionSize(38);
  m_table->setMinimumWidth(430);
  m_table->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Interactive);
  m_table->horizontalHeader()->setMinimumSectionSize(70);
  for (int c = 1; c < 7; ++c)
    m_table->setColumnWidth(c, c == 5 ? 140 : 85);
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  split->addWidget(m_table);
  auto *right = new QWidget;
  auto *form = new QVBoxLayout(right);
  m_preview = new FitImageView;
  m_preview->clear("导入素材后，选择左侧图片开始审核");
  m_preview->setMinimumSize(280, 180);
  m_preview->setMaximumHeight(240);
  form->addWidget(m_preview, 1);
  m_path = new QLabel;
  m_path->setWordWrap(true);
  m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);
  form->addWidget(m_path);
  m_metadata = new QLabel;
  m_metadata->setWordWrap(true);
  m_metadata->setStyleSheet("color:#667a94;font-size:9pt;");
  form->addWidget(m_metadata);
  auto *previewTools = new QHBoxLayout;
  auto *enlarge = new QPushButton("缩放 / 平移预览");
  auto *locate = new QPushButton("打开所在目录");
  previewTools->addWidget(enlarge);
  previewTools->addWidget(locate);
  form->addLayout(previewTools);
  connect(enlarge, &QPushButton::clicked, this,
          &ReviewWorkspace::enlargePreview);
  connect(locate, &QPushButton::clicked, this, [this] {
    if (!m_currentPath.isEmpty())
      QDesktopServices::openUrl(
          QUrl::fromLocalFile(QFileInfo(m_currentPath).absolutePath()));
  });
  auto *fields = new QFormLayout;
  m_verdict = new QComboBox;
  m_verdict->setObjectName("reviewVerdict");
  for (const auto &s : QStringList{"pending", "pass", "fail"})
    m_verdict->addItem(label(s), s);
  m_reviewer = new QLineEdit(QSettings().value("reviewer").toString());
  m_reviewer->setObjectName("reviewerName");
  m_reviewer->setMaxLength(64);
  m_defect = new QLineEdit;
  m_defect->setObjectName("reviewReason");
  m_defect->setMaxLength(100);
  m_defect->setPlaceholderText("退回必填，例如：尺寸不符、文案错误、图片模糊");
  m_tags = new QLineEdit;
  m_tags->setObjectName("reviewTags");
  m_tags->setMaxLength(200);
  m_tags->setPlaceholderText("逗号分隔，例如：双十一,主图,已授权");
  m_note = new QTextEdit;
  m_note->setMaximumHeight(100);
  fields->addRow("审核状态", m_verdict);
  fields->addRow("审核人（本机登记）", m_reviewer);
  fields->addRow("退回原因", m_defect);
  fields->addRow("素材标签", m_tags);
  fields->addRow("备注（最多2000字）", m_note);
  form->addLayout(fields);
  m_continue = new QCheckBox("保存后继续审核下一张待审核素材");
  m_continue->setObjectName("reviewContinue");
  m_continue->setChecked(true);
  form->addWidget(m_continue);
  auto *quick = new QHBoxLayout;
  for (const auto &reason : QStringList{"尺寸不符", "文案错误", "图片模糊"}) {
    auto *button = new QPushButton(reason);
    quick->addWidget(button);
    connect(button, &QPushButton::clicked, this, [this, reason] {
      m_verdict->setCurrentIndex(m_verdict->findData("fail"));
      m_defect->setText(reason);
    });
  }
  form->addLayout(quick);
  connect(m_verdict, &QComboBox::currentIndexChanged, this, [this] {
    const bool rejected = m_verdict->currentData().toString() == "fail";
    m_defect->setEnabled(rejected);
    if (!rejected) m_defect->clear();
  });
  auto *buttons = new QHBoxLayout;
  m_save = new QPushButton("保存审核");
  m_save->setProperty("primary", true);
  m_save->setObjectName("saveReview");
  auto *audit = new QPushButton("修改历史");
  buttons->addWidget(m_save);
  buttons->addWidget(audit);
  auto *detailsScroll = new QScrollArea;
  detailsScroll->setFrameShape(QFrame::NoFrame);
  detailsScroll->setWidgetResizable(true);
  detailsScroll->setMinimumWidth(350);
  detailsScroll->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
  detailsScroll->setWidget(right);
  auto *detailsContainer = new QWidget;
  auto *detailsLayout = new QVBoxLayout(detailsContainer);
  detailsLayout->setContentsMargins(0, 0, 0, 0);
  detailsLayout->addWidget(detailsScroll, 1);
  detailsLayout->addLayout(buttons);
  split->addWidget(detailsContainer);
  split->setStretchFactor(0, 3);
  split->setStretchFactor(1, 2);
  split->setSizes({650, 450});
  layout->addWidget(split, 1);
  auto *footer = new QHBoxLayout;
  m_previous = new QPushButton("上一页");
  m_next = new QPushButton("下一页");
  m_info = new QLabel;
  footer->addWidget(m_previous);
  footer->addWidget(m_next);
  footer->addWidget(m_info, 1);
  layout->addLayout(footer);
  auto *notice =
      new QLabel("登记原图路径，不移动、不删除源文件；源图需保留在原"
                 "位置。审核结果仅用于本工具，不代表广告平台审核结果。");
  notice->setWordWrap(true);
  layout->addWidget(notice);
  connect(directoryAction, &QAction::triggered, this,
          &ReviewWorkspace::importDirectory);
  connect(filesAction, &QAction::triggered, this, [this] {
    const auto files = QFileDialog::getOpenFileNames(
        this, "选择营销素材", {}, "图片 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (!files.isEmpty())
      beginImport(files);
  });
  connect(appendAction, &QAction::triggered, this, [this] {
    if (batchId() < 0)
      return;
    const auto files = QFileDialog::getOpenFileNames(
        this, "追加素材", {}, "图片 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (!files.isEmpty())
      beginImport(files, true);
  });
  connect(bulk, &QPushButton::clicked, this, &ReviewWorkspace::batchReview);
  connect(log, &QPushButton::clicked, this, &ReviewWorkspace::showImportLog);
  connect(m_cancelButton, &QPushButton::clicked, this, [this] {
    if (m_cancel)
      m_cancel->store(true);
    m_cancelButton->setEnabled(false);
  });
  connect(m_batches, &QComboBox::currentIndexChanged, this, [this] {
    m_offset = 0;
    refresh();
  });
  connect(m_filter, &QComboBox::currentIndexChanged, this, [this] {
    m_offset = 0;
    refresh();
  });
  connect(search, &QPushButton::clicked, this, [this] {
    m_offset = 0;
    refresh();
  });
  connect(m_key, &QLineEdit::returnPressed, search, &QPushButton::click);
  connect(m_previous, &QPushButton::clicked, this, [this] {
    m_offset = qMax(0, m_offset - 100);
    refresh();
  });
  connect(m_next, &QPushButton::clicked, this, [this] {
    m_offset += 100;
    refresh();
  });
  connect(m_table, &QTableWidget::itemSelectionChanged, this,
          &ReviewWorkspace::selected);
  connect(m_table, &QTableWidget::itemSelectionChanged, this, [this, bulk] {
    const auto count = m_table->selectionModel()->selectedRows().size();
    bulk->setEnabled(count > 0);
    bulk->setText(count ? QString("批量审核（%1张）").arg(count) : "批量审核");
  });
  connect(m_table, &QTableWidget::cellDoubleClicked, this,
          [this](int, int) { enlargePreview(); });
  connect(m_save, &QPushButton::clicked, this, &ReviewWorkspace::saveReview);
  connect(audit, &QPushButton::clicked, this, &ReviewWorkspace::history);
  connect(exportButton, &QPushButton::clicked, this, [this] {
    if (batchId() < 0)
      return;
    QString file = QFileDialog::getSaveFileName(this, "导出审核结果",
        "素材审核-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".csv", "CSV (*.csv)");
    if (file.isEmpty())
      return;
    if (!file.endsWith(".csv", Qt::CaseInsensitive)) file += ".csv";
    if (!m_repo.exportCsv(file, batchId(), statusFilter(),
                          m_key->text().trimmed()))
      error(m_repo.lastError());
    else
      m_info->setText("导出成功：" + file);
  });
  if (!m_repo.open()) {
    m_info->setText("数据库打开失败：" + m_repo.lastError());
    setEnabled(false);
    return;
  }
  m_info->setToolTip(dbPath());
  loadBatches();
}
ReviewWorkspace::~ReviewWorkspace() {
  if (m_cancel)
    m_cancel->store(true);
  if (m_thread) {
    m_thread->quit();
    m_thread->wait();
  }
  QThreadPool::globalInstance()->waitForDone();
}
qint64 ReviewWorkspace::batchId() const {
  return m_batches->currentIndex() < 0 ? -1
                                       : m_batches->currentData().toLongLong();
}
QString ReviewWorkspace::statusFilter() const {
  return m_filter->currentData().toString();
}
void ReviewWorkspace::error(const QString &message) {
  QMessageBox::warning(this, "操作失败", message);
}
void ReviewWorkspace::loadBatches(qint64 selected) {
  QSignalBlocker blocker(m_batches);
  m_batches->clear();
  for (const auto &b : m_repo.batches())
    m_batches->addItem(b.value("product").toString() + " / " +
                           b.value("channel").toString() + " / " +
                           b.value("number").toString(),
                       b.value("id"));
  int index = m_batches->findData(selected);
  if (index >= 0)
    m_batches->setCurrentIndex(index);
  m_offset = 0;
  refresh();
}
void ReviewWorkspace::refresh() {
  const auto stats =
      batchId() < 0 ? QVariantMap{} : m_repo.statistics(batchId());
  for (int i = 0; i < m_metrics.size(); ++i)
    m_metrics[i]->setText(QString::number(
        stats.value(QStringList{"total", "pending", "pass", "fail"}[i])
            .toInt()));
  ++m_generation;
  findChild<QPushButton *>("bulkReview")->setEnabled(false);
  m_currentPath.clear();
  m_selected.clear();
  m_save->setEnabled(false);
  m_preview->clear("请选择素材进行审核");
  m_path->clear();
  m_metadata->clear();
  m_defect->clear();
  m_tags->clear();
  m_note->clear();
  QSignalBlocker blocker(m_table);
  m_table->setRowCount(0);
  m_rows.clear();
  const int total = batchId() < 0 ? 0
                                  : m_repo.count(batchId(), statusFilter(),
                                                 m_key->text().trimmed());
  if (total < 0) {
    m_info->setText(m_repo.lastError());
    return;
  }
  if (m_offset >= total && m_offset > 0)
    m_offset = qMax(0, ((total - 1) / 100) * 100);
  if (batchId() >= 0)
    m_rows = m_repo.images(batchId(), statusFilter(), m_key->text().trimmed(),
                           m_offset);
  for (const auto &r : m_rows) {
    const int i = m_table->rowCount();
    m_table->insertRow(i);
    QStringList values{QFileInfo(r.value("path").toString()).fileName(),
                       label(r.value("status").toString()),
                       r.value("tags").toString(),
                       r.value("defect").toString(),
                       r.value("reviewer").toString(),
                       r.value("reviewed_at").toString(),
                       QFileInfo::exists(r.value("path").toString()) ? "存在"
                                                                     : "缺失"};
    for (int c = 0; c < values.size(); ++c)
      m_table->setItem(i, c, new QTableWidgetItem(values[c]));
    m_table->item(i, 0)->setToolTip(r.value("path").toString());
    const auto status = r.value("status").toString();
  m_table->item(i, 1)->setForeground(QColor(status == "pass"   ? "#13836d"
                                              : status == "fail" ? "#cf6151"
                                                                 : "#b37a22"));
  }
  m_previous->setEnabled(m_offset > 0);
  m_next->setEnabled(m_offset + 100 < total);
  m_info->setText(QString("共 %1 张，当前第 %2 页，每页100张")
                      .arg(total)
                      .arg(m_offset / 100 + 1));
  if (batchId() < 0) {
    m_info->setText("尚未创建素材批次，点击“导入素材”或将图片目录拖入窗口开始");
    m_preview->clear("导入 → 预检 → 审核 → 导出结果");
  } else if (total == 0) {
    m_preview->clear("当前筛选下没有素材，请调整状态或搜索条件");
  }
}
void ReviewWorkspace::selected() {
  const int r = m_table->currentRow();
  if (r < 0 || r >= m_rows.size())
    return;
  m_selected = m_rows[r];
  ++m_generation;
  m_currentPath = m_selected.value("path").toString();
  m_path->setText(m_currentPath);
  m_metadata->setText(
      QString("%1 × %2 px  ·  %3  ·  %4 MiB\n文件修改时间：%5")
          .arg(m_selected.value("width").toInt())
          .arg(m_selected.value("height").toInt())
          .arg(m_selected.value("format").toString())
          .arg(m_selected.value("bytes").toLongLong() / 1048576.0, 0, 'f', 2)
          .arg(m_selected.value("modified_at").toString()));
  m_note->setPlainText(m_selected.value("note").toString());
  m_defect->setText(m_selected.value("defect").toString());
  m_tags->setText(m_selected.value("tags").toString());
  m_verdict->setCurrentIndex(m_verdict->findData(m_selected.value("status")));
  m_save->setEnabled(false);
  m_preview->clear("正在加载预览…");
  if (!QFileInfo::exists(m_currentPath)) {
    m_preview->clear("原图缺失，请恢复文件后刷新");
    return;
  }
  m_preview->clear("解码中…");
  const QFileInfo f(m_currentPath);
  const QString key = "review:" + m_currentPath + ":" +
                      QString::number(f.size()) + ":" +
                      QString::number(f.lastModified().toMSecsSinceEpoch());
  const auto cached = ThumbCache::Instance().Get(key);
  if (!cached.isNull()) {
    m_preview->setImage(cached);
    m_save->setEnabled(true);
    return;
  }
  auto *task = new ThumbnailTask(m_currentPath, 1024, m_generation);
  connect(task, &ThumbnailTask::sigThumbReady, this,
          &ReviewWorkspace::previewReady, Qt::QueuedConnection);
  QThreadPool::globalInstance()->start(task);
}
void ReviewWorkspace::previewReady(int gen, const QString &path,
                                   const QImage &image) {
  if (gen != m_generation || path != m_currentPath)
    return;
  if (image.isNull()) {
    m_preview->clear("图片无法解码");
    return;
  }
  const QFileInfo f(path);
  auto pix = QPixmap::fromImage(image);
  ThumbCache::Instance().Put(
      "review:" + path + ":" + QString::number(f.size()) + ":" +
          QString::number(f.lastModified().toMSecsSinceEpoch()),
      pix);
  m_preview->setImage(pix);
  m_save->setEnabled(true);
}
void ReviewWorkspace::saveReview() {
  if (m_selected.isEmpty())
    return;
  if (m_reviewer->text().trimmed().isEmpty()) {
    error("请先填写审核人"); m_reviewer->setFocus(); return;
  }
  if (m_verdict->currentData().toString() == "fail" && m_defect->text().trimmed().isEmpty()) {
    error("退回素材必须填写原因，可点击常用原因快捷填写"); m_defect->setFocus(); return;
  }
  const bool continueReview = m_continue->isChecked() && m_verdict->currentData().toString() != "pending";
  qint64 nextId = -1;
  const int current = m_table->currentRow();
  if (continueReview) {
    for (int step = 1; step < m_rows.size(); ++step) {
      const auto &r = m_rows[(current + step) % m_rows.size()];
      if (r.value("status").toString() == "pending") { nextId = r.value("id").toLongLong(); break; }
    }
  }
  if (!m_repo.review(m_selected.value("id").toLongLong(),
                     m_verdict->currentData().toString(), m_defect->text(),
                     m_note->toPlainText(), m_tags->text(), m_reviewer->text(),
                     m_selected.value("version").toInt())) {
    error(m_repo.lastError());
    return;
  }
  QSettings().setValue("reviewer", m_reviewer->text().trimmed());
  const qint64 id = m_selected.value("id").toLongLong();
  refresh();
  bool selectedNext = false;
  for (int i = 0; i < m_rows.size(); ++i)
    if (m_rows[i].value("id").toLongLong() == (continueReview ? nextId : id)) {
      m_table->selectRow(i);
      selectedNext = true;
      break;
    }
  // A filtered page may shrink after review; load the next pending item
  // instead of leaving a blank inspector between consecutive decisions.
  if (continueReview && !selectedNext) {
    if (statusFilter() != "pending") {
      m_filter->setCurrentIndex(m_filter->findData("pending"));
    }
    if (!m_rows.isEmpty()) m_table->selectRow(0);
  }
  m_info->setText(continueReview && m_selected.isEmpty()
      ? "审核已保存；当前筛选下的待审核素材已处理完毕"
      : "审核已保存" + QString(continueReview ? "，已定位下一张待审核素材" : ""));
}
void ReviewWorkspace::history() {
  if (m_selected.isEmpty())
    return;
  QDialog dialog(this);
  dialog.setWindowTitle("审核修改历史（仅本机操作留痕，不代表身份认证）");
  auto *layout = new QVBoxLayout(&dialog);
  auto *text = new QPlainTextEdit;
  text->setReadOnly(true);
  for (const auto &r : m_repo.history(m_selected.value("id").toLongLong()))
    text->appendPlainText(QString("%1  %2\n修改前：%3\n修改后：%4\n")
                              .arg(r.value("created_at").toString(),
                                   r.value("reviewer").toString(),
                                   r.value("old_value").toString(),
                                   r.value("new_value").toString()));
  layout->addWidget(text);
  dialog.resize(850, 500);
  dialog.exec();
}
void ReviewWorkspace::importDirectory() {
  if (m_thread)
    return;
  const QString root = QFileDialog::getExistingDirectory(
      this, "选择批次原图目录（只登记，不移动）");
  if (root.isEmpty())
    return;
  beginImport({root});
}
void ReviewWorkspace::beginImport(const QStringList &sources, bool append) {
  if (m_thread || sources.isEmpty())
    return;
  const QFileInfo first(sources.first());
  const QString root =
      first.isDir() ? first.absoluteFilePath() : first.absolutePath();
  qint64 id = batchId();
  if (!append) {
    QDialog dialog(this);
    dialog.setWindowTitle("登记营销素材批次");
    dialog.resize(500, 280);
    auto *form = new QFormLayout(&dialog);
    QLineEdit product, number;
    QComboBox channel;
    channel.setEditable(true);
    channel.addItems({"信息流", "搜索广告", "内容渠道", "其他"});
    product.setMaxLength(100);
    product.setText(QFileInfo(root).fileName());
    number.setMaxLength(100);
    channel.lineEdit()->setMaxLength(100);
    number.setText(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
    form->addRow("投放项目", &product);
    form->addRow("投放渠道", &channel);
    form->addRow("素材批次", &number);
    auto *source = new QLabel(
        QString("来源：%1\n预检内容、尺寸与体积后登记；原文件保留原位置。")
            .arg(root));
    source->setWordWrap(true);
    form->addRow(source);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(&buttons);
    auto *validation = new QLabel;
    validation->setStyleSheet("color:#be463b;");
    form->addRow(validation);
    buttons.button(QDialogButtonBox::Ok)->setText("开始预检并导入");
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
      if (product.text().trimmed().isEmpty() || number.text().trimmed().isEmpty()
          || channel.currentText().trimmed().isEmpty()) {
        validation->setText("请填写投放项目、渠道和批次号"); return;
      }
      dialog.accept();
    });
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
      return;
    id = m_repo.createBatch(product.text(), number.text(), root,
                            channel.currentText());
  }
  if (id < 0) {
    error(m_repo.lastError());
    return;
  }
  loadBatches(id);
  m_imported = 0;
  m_accepted = 0;
  m_rejected = 0;
  m_importIssues.clear();
  m_progress->setVisible(true);
  m_importError.clear();
  m_import->setEnabled(false);
  m_batches->setEnabled(false);
  m_cancelButton->setEnabled(true);
  m_cancel = std::make_shared<std::atomic_bool>(false);
  m_thread = new QThread(this);
  auto *worker = new ReviewScanWorker(root, m_cancel, sources);
  worker->moveToThread(m_thread);
  connect(m_thread, &QThread::started, worker, &ReviewScanWorker::run);
  connect(worker, &ReviewScanWorker::batch, this,
          [this, id](const QList<QVariantMap> &paths) {
            if (!m_importError.isEmpty())
              return;
            int n = 0;
            if (!m_repo.addValidatedImages(id, paths, &n)) {
              if (m_cancel)
                m_cancel->store(true);
              m_importError = "登记失败：" + m_repo.lastError();
              m_info->setText(m_importError);
              return;
            }
            m_imported += n;
            m_info->setText(
                QString("已新增 %1 张；重复图片自动跳过").arg(m_imported));
          });
  connect(worker, &ReviewScanWorker::issue, this,
          [this](const QString &path, const QString &reason) {
            if (m_importIssues.size() < 500)
              m_importIssues << QFileInfo(path).fileName() + "：" + reason;
          });
  connect(worker, &ReviewScanWorker::progress, this,
          [this](int scanned, int accepted, int rejected) {
            m_accepted = accepted;
            m_rejected = rejected;
            m_info->setText(QString("已预检 %1 张，有效 %2，跳过 %3；已入库 %4")
                                .arg(scanned)
                                .arg(accepted)
                                .arg(rejected)
                                .arg(m_imported));
          });
  connect(worker, &ReviewScanWorker::finished, m_thread, &QThread::quit,
          Qt::DirectConnection);
  connect(m_thread, &QThread::finished, worker, &QObject::deleteLater);
  connect(m_thread, &QThread::finished, this, [this] {
    const bool cancelled = m_cancel->load();
    m_thread->deleteLater();
    m_thread = nullptr;
    m_import->setEnabled(true);
    m_batches->setEnabled(true);
    m_cancelButton->setEnabled(false);
    m_progress->setVisible(false);
    refresh();
    if (!m_importError.isEmpty()) {
      m_info->setText(m_importError);
      return;
    }
    m_info->setText(QString("%1：新增 %2 张，路径重复 %3，跳过 %4；原图未修改")
                        .arg(cancelled ? "扫描已取消" : "扫描完成")
                        .arg(m_imported)
                        .arg(qMax(0, m_accepted - m_imported))
                        .arg(m_rejected));
  });
  m_thread->start();
}
void ReviewWorkspace::dragEnterEvent(QDragEnterEvent *event) {
  if (!m_thread && event->mimeData()->hasUrls())
    for (const auto &url : event->mimeData()->urls())
      if (url.isLocalFile()) {
        event->acceptProposedAction();
        return;
      }
}
void ReviewWorkspace::dropEvent(QDropEvent *event) {
  if (m_thread)
    return;
  QStringList sources;
  for (const auto &url : event->mimeData()->urls())
    if (url.isLocalFile())
      sources << url.toLocalFile();
  if (!sources.isEmpty()) {
    event->acceptProposedAction();
    QTimer::singleShot(0, this, [this, sources] { beginImport(sources); });
  }
}
void ReviewWorkspace::showImportLog() {
  QDialog dialog(this);
  dialog.setWindowTitle("最近一次导入记录");
  dialog.resize(760, 440);
  auto *layout = new QVBoxLayout(&dialog);
  auto *summary = new QLabel(
      QString("新增 %1，路径重复 %2，跳过 %3。最多显示500条跳过原因。")
          .arg(m_imported)
          .arg(qMax(0, m_accepted - m_imported))
          .arg(m_rejected));
  summary->setWordWrap(true);
  layout->addWidget(summary);
  auto *text = new QPlainTextEdit;
  text->setReadOnly(true);
  text->setPlainText(m_importIssues.isEmpty() ? "没有跳过记录"
                                              : m_importIssues.join('\n'));
  layout->addWidget(text);
  dialog.exec();
}
void ReviewWorkspace::batchReview() {
  QList<QVariantMap> selected;
  for (const auto &index : m_table->selectionModel()->selectedRows())
    if (index.row() < m_rows.size())
      selected << m_rows[index.row()];
  if (selected.isEmpty()) {
    error("请先选择素材，可用 Ctrl 或 Shift 多选");
    return;
  }
  QDialog dialog(this);
  dialog.setWindowTitle("批量审核");
  auto *form = new QFormLayout(&dialog);
  QComboBox verdict;
  verdict.addItem("通过", "pass");
  verdict.addItem("退回", "fail");
  QLineEdit who(m_reviewer->text()), reason;
  QLineEdit tags;
  QTextEdit note;
  who.setMaxLength(64);
  reason.setMaxLength(100);
  form->addRow(new QLabel(
      QString("已选择 %1 张；任一记录失败将整体回滚。").arg(selected.size())));
  form->addRow("审核结果", &verdict);
  form->addRow("审核人", &who);
  form->addRow("退回原因", &reason);
  form->addRow("素材标签", &tags);
  form->addRow("备注", &note);
  QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  form->addRow(&buttons);
  auto *validation = new QLabel;
  validation->setStyleSheet("color:#be463b;");
  form->addRow(validation);
  connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
    if (who.text().trimmed().isEmpty() || tags.text().size() > 200 || note.toPlainText().size() > 2000 ||
        (verdict.currentData().toString() == "fail" && reason.text().trimmed().isEmpty())) {
      validation->setText("请填写审核人；退回须填写原因；标签不超过200字；备注不超过2000字"); return;
    }
    dialog.accept();
  });
  connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  dialog.resize(480, 360);
  if (dialog.exec() != QDialog::Accepted)
    return;
  for (auto &row : selected)
    row["tags"] = tags.text().trimmed();
  if (!m_repo.reviewMany(selected, verdict.currentData().toString(),
                         reason.text(), note.toPlainText(), who.text())) {
    error(m_repo.lastError());
    return;
  }
  m_reviewer->setText(who.text());
  refresh();
  m_info->setText(
      QString("已批量审核 %1 张，审核记录已保存").arg(selected.size()));
}
void ReviewWorkspace::enlargePreview() {
  if (m_currentPath.isEmpty() || !QFileInfo::exists(m_currentPath))
    return;
  QDialog dialog(this);
  dialog.setWindowTitle("图片预览 · 滚轮缩放 · 拖动平移 · 双击适应");
  dialog.resize(1050, 760);
  auto *layout = new QVBoxLayout(&dialog);
  auto *canvas = new ImageCanvas;
  layout->addWidget(canvas, 1);
  auto *message = new QLabel("正在读取预览…");
  layout->addWidget(message);
  auto *fit = new QPushButton("适应窗口");
  layout->addWidget(fit);
  connect(fit, &QPushButton::clicked, canvas, &ImageCanvas::fit);
  auto *task = new ThumbnailTask(m_currentPath, 4096, 0);
  connect(
      task, &ThumbnailTask::sigThumbReady, &dialog,
      [canvas, message](int, const QString &, const QImage &image) {
        if (image.isNull()) {
          message->setText("无法解码，请确认源文件仍存在");
          return;
        }
        canvas->setImage(image);
        message->setText(
            QString("预览 %1 × %2 px；为控制内存，最长边不超过4096像素")
                .arg(image.width())
                .arg(image.height()));
      },
      Qt::QueuedConnection);
  QThreadPool::globalInstance()->start(task);
  dialog.exec();
}
