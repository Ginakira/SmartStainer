//
// Created by Ginakira on 2023/6/18.
//

#ifndef SMARTSTAINER__STAINING_SCHEME_GENERATOR_H_
#define SMARTSTAINER__STAINING_SCHEME_GENERATOR_H_

#include <QMap>
#include <QObject>
#include <QSet>

constexpr static double SIMILARITY_INF = std::numeric_limits<double>::max();

struct StainingGroup {
  QString antibody;
  QString spectrum;
  QString channel;
};

struct StainingSchemeResult {
  QList<StainingGroup> staining_groups;
  double similarity{SIMILARITY_INF};
};

using StainingSchemeResultList = QList<StainingSchemeResult>;
using StainingMap =
    QMap<QString /* antibody */,
         QMap<QString /* spectrum */, QSet<QString> /* channels */>>;
using ChannelSimilarityMap =
    QMap<QString /* channel */,
         QMap<QString /* channel */, double /* similarity */>>;

class StainingSchemeGenerator : public QObject {
  Q_OBJECT
 public:
  explicit StainingSchemeGenerator(QObject *parent, QString filename);
  ~StainingSchemeGenerator() override;

  bool LoadLibraryFile();
  bool LoadSimilarityFile(const QString &filename);

 public slots:
  void SelectAntibody(const QString &antibody);
  void DeselectAntibody(const QString &antibody);
  void GenerateSchemes();

 signals:
  void LibraryFileLoaded(bool success, QString filename, int lines_count);
  void SimilarityFileLoaded(bool success, QString filename, int lines_count);
  void AvailableAntibodiesRefreshed(QStringList antibodies);
  void SelectedAntibodiesRefreshed(QStringList antibodies);
  void SchemesGenerated(StainingSchemeResultList scheme_result_list);

 private:
  void SchemeBacktrace(const QStringList &selected_antibodies,
                       QSet<QString> &used_spectrum,
                       StainingSchemeResultList &result_list,
                       StainingSchemeResult &cur_result, int cur_index);
  void CalculateSimilarity(StainingSchemeResult &scheme);

 private:
  QString filename_;
  QSet<QString> selected_antibodies_;
  QSet<QString> available_antibodies_;
  StainingMap antibody_spectrum_to_channels_;
  bool similarity_file_loaded_{false};
  ChannelSimilarityMap channel_to_similarity_;
};

#endif  // SMARTSTAINER__STAINING_SCHEME_GENERATOR_H_
