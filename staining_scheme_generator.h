//
// Created by Ginakira on 2023/6/18.
//

#ifndef SMARTSTAINER__STAINING_SCHEME_GENERATOR_H_
#define SMARTSTAINER__STAINING_SCHEME_GENERATOR_H_

#include <QMap>
#include <QSet>

struct StainingGroup {
  QString antibody;
  QString spectrum;
  QString channel;
};

using StainingSchemeResult = QList<StainingGroup>;
using StainingSchemeResultList = QList<StainingSchemeResult>;
using StainingMap = QMap<QString, QMap<QString, QSet<QString>>>;

class StainingSchemeGenerator : public QObject {
 Q_OBJECT
 public:
  explicit StainingSchemeGenerator(QObject *parent, QString filename);
  ~StainingSchemeGenerator() override;

  bool LoadFile();

 public slots:
  void SelectAntibody(const QString &antibody);
  void DeselectAntibody(const QString &antibody);
  void GenerateSchemes();

 signals:
  void FileLoaded(bool success, QString filename, int lines_count);
  void AvailableAntibodiesRefreshed(QStringList antibodies);
  void SelectedAntibodiesRefreshed(QStringList antibodies);
  void SchemesGenerated(StainingSchemeResultList scheme_result_list);

 private:
  QString filename_;
  QSet<QString> selected_antibodies_;
  QSet<QString> available_antibodies_;
  StainingMap antibody_spectrum_to_channels_;
  void SchemeBacktrace(const QStringList &selected_antibodies,
                       QSet<QString> &used_spectrum,
                       StainingSchemeResultList &result_list,
                       StainingSchemeResult &cur_result,
                       int cur_index);
};

#endif //SMARTSTAINER__STAINING_SCHEME_GENERATOR_H_
