#include "main_window.h"

#include <QFileDialog>
#include <QMessageBox>

#include "ui/ui_main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  connect(ui->library_file_import_button, &QPushButton::clicked, this,
          &MainWindow::ImportLibraryFile);
  connect(ui->similarity_file_import_button, &QPushButton::clicked, this,
          &MainWindow::ImportSimilarityFile);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::ImportLibraryFile() {
  QString cur_path = QCoreApplication::applicationDirPath();
  QString filename =
      QFileDialog::getOpenFileName(this, "打开库文件", cur_path, "文本(*.txt)");

  if (filename.isEmpty()) {
    return;
  }

  SetupStainingSchemeGenerator(filename);
  Q_ASSERT(scheme_generator_);
  scheme_generator_->LoadLibraryFile();
}

void MainWindow::ImportSimilarityFile() {
  if (!scheme_generator_) {
    QMessageBox::critical(this, "Error", "请先导入库文件");
    return;
  }
  QString cur_path = QCoreApplication::applicationDirPath();
  QString filename = QFileDialog::getOpenFileName(this, "打开相似度文件",
                                                  cur_path, "文本(*.txt)");

  if (filename.isEmpty()) {
    return;
  }

  Q_ASSERT(scheme_generator_);
  scheme_generator_->LoadSimilarityFile(filename);
}

void MainWindow::SetupStainingSchemeGenerator(const QString &filename) {
  if (scheme_generator_) {
    scheme_generator_->disconnect();
    this->disconnect(scheme_generator_);
    scheme_generator_->deleteLater();
  }
  scheme_generator_ = new StainingSchemeGenerator(this, filename);

  connect(scheme_generator_, &StainingSchemeGenerator::LibraryFileLoaded, this,
          &MainWindow::LibraryFileLoadedHandler);
  connect(scheme_generator_, &StainingSchemeGenerator::SimilarityFileLoaded,
          this, &MainWindow::SimilarityFileLoadedHandler);
  connect(scheme_generator_,
          &StainingSchemeGenerator::AvailableAntibodiesRefreshed, this,
          &MainWindow::RefreshAvailableAntibodies);
  connect(scheme_generator_,
          &StainingSchemeGenerator::SelectedAntibodiesRefreshed, this,
          &MainWindow::RefreshSelectedAntibodies);
  connect(scheme_generator_, &StainingSchemeGenerator::SchemesGenerated, this,
          &MainWindow::ShowSchemes);

  connect(ui->add_antibody_button, &QPushButton::clicked, this,
          &MainWindow::AddAntibody);
  connect(ui->remove_antibody_button, &QPushButton::clicked, this,
          &MainWindow::RemoveAntibody);
  connect(ui->generate_button, &QPushButton::clicked, this,
          &MainWindow::GenerateSchemes);
}

void MainWindow::LibraryFileLoadedHandler(bool success, const QString &filename,
                                          int lines_count) {
  if (!success) {
    QMessageBox::critical(this, "Error", "库文件导入失败！");
  }
  ui->library_file_name_label->setText(
      QString("库文件(%1) %2").arg(QString::number(lines_count), filename));
  ui->statusbar->showMessage(QString("已成功导入%1条库数据").arg(lines_count));
}

void MainWindow::SimilarityFileLoadedHandler(bool success,
                                             const QString &filename,
                                             int lines_count) {
  if (!success) {
    QMessageBox::critical(this, "Error", "相似度文件导入失败！");
  }
  ui->similarity_file_name_label->setText(
      QString("相似度文件(%1) %2").arg(QString::number(lines_count), filename));
  ui->statusbar->showMessage(
      QString("已成功导入%1条相似度数据").arg(lines_count));
}

void MainWindow::RefreshAvailableAntibodies(const QStringList &antibodies) {
  ui->available_antibody_list_widget->clear();
  ui->available_antibody_list_widget->addItems(antibodies);
  ui->available_antibody_list_widget->sortItems();
}

void MainWindow::RefreshSelectedAntibodies(const QStringList &antibodies) {
  ui->selected_antibody_list_widget->clear();
  ui->selected_antibody_list_widget->addItems(antibodies);
  ui->selected_antibody_list_widget->sortItems();
}

void MainWindow::AddAntibody() {
  Q_ASSERT(scheme_generator_);
  auto antibody_item = ui->available_antibody_list_widget->currentItem();
  if (!antibody_item) {
    return;
  }
  ui->statusbar->showMessage(QString("选择抗体 %1").arg(antibody_item->text()));
  scheme_generator_->SelectAntibody(antibody_item->text());
}

void MainWindow::RemoveAntibody() {
  Q_ASSERT(scheme_generator_);
  auto antibody_item = ui->selected_antibody_list_widget->currentItem();
  if (!antibody_item) {
    return;
  }
  ui->statusbar->showMessage(QString("移除抗体 %1").arg(antibody_item->text()));
  scheme_generator_->DeselectAntibody(antibody_item->text());
}

void MainWindow::GenerateSchemes() {
  Q_ASSERT(scheme_generator_);
  ui->statusbar->showMessage("正在生成染色方案...");
  scheme_generator_->GenerateSchemes();
}

void MainWindow::ShowSchemes(
    const StainingSchemeResultList &scheme_result_list) {
  ui->generate_result_text_edit->clear();
  for (int i = 0; const auto &result : scheme_result_list) {
    ++i;
    auto similarity = result.similarity == SIMILARITY_INF
                          ? "INF"
                          : QString::number(result.similarity);
    ui->generate_result_text_edit->append(
        QString("Scheme #%1, MaxSimilarity: %2")
            .arg(QString::number(i), similarity));
    for (auto &item : result.staining_groups) {
      ui->generate_result_text_edit->append(
          QString("%1, %2, %3")
              .arg(item.spectrum, item.channel, item.antibody));
    }
    ui->generate_result_text_edit->append("--------------------------------");
  }
  ui->statusbar->showMessage(
      QString("找到有效的染色方案共 %1 种").arg(scheme_result_list.size()));
}
