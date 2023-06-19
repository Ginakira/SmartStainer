#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "staining_scheme_generator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
 Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

 private:
  void ImportFile();
  void SetupStainingSchemeGenerator(const QString &filename);

 private slots:
  void FileLoadedHandler(bool success, const QString &filename, int lines_count);
  void RefreshAvailableAntibodies(const QStringList &antibodies);
  void RefreshSelectedAntibodies(const QStringList &antibodies);
  void AddAntibody();
  void RemoveAntibody();
  void GenerateSchemes();
  void ShowSchemes(const StainingSchemeResultList &scheme_result_list);


 private:
  Ui::MainWindow *ui;
  StainingSchemeGenerator *scheme_generator_ = nullptr;
};
#endif // MAINWINDOW_H
