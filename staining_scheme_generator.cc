//
// Created by Ginakira on 2023/6/18.
//

#include "staining_scheme_generator.h"

#include <QDebug>
#include <QFile>
#include <QString>

StainingSchemeGenerator::StainingSchemeGenerator(QObject *parent,
                                                 QString filename)
    : QObject(parent), filename_(std::move(filename)) {}

StainingSchemeGenerator::~StainingSchemeGenerator() = default;

bool StainingSchemeGenerator::LoadLibraryFile() {
  QFile file(filename_);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    emit LibraryFileLoaded(false, "", -1);
    return false;
  }

  int loaded_lines = 0, available_lines = 0;
  while (!file.atEnd()) {
    auto line = file.readLine();
    if (++loaded_lines == 1) {
      // Ignore the first line because it is sheet header
      continue;
    }

    auto items = line.split(',');
    if (items.size() < 3) {
      qWarning() << "[LibraryFile] Line" << loaded_lines << "is too short!";
      continue;
    }
    QString spectrum = items[0].trimmed();
    QString fluorescence_channel = items[1].trimmed();
    QString anti_body = items[2].trimmed();

    if (spectrum.isEmpty() || fluorescence_channel.isEmpty() ||
        anti_body.isEmpty()) {
      qWarning() << "[LibraryFile] Line" << loaded_lines
                 << "content is incorrect!";
      continue;
    }

    antibody_spectrum_to_channels_[anti_body][spectrum].insert(
        fluorescence_channel);
    available_antibodies_.insert(anti_body);

    qInfo() << "[LibraryFile] Line" << loaded_lines
            << "loaded. spectrum:" << spectrum
            << "fluorescence channel:" << fluorescence_channel
            << "anti body:" << anti_body;
    ++available_lines;
  }

  emit LibraryFileLoaded(true, filename_, available_lines);
  emit AvailableAntibodiesRefreshed(available_antibodies_.values());
  return true;
}

bool StainingSchemeGenerator::LoadSimilarityFile(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    emit SimilarityFileLoaded(false, "", -1);
    return false;
  }
  spectrum_to_similarity_.clear();

  int loaded_lines = 0, available_lines = 0;
  while (!file.atEnd()) {
    auto line = file.readLine();
    ++loaded_lines;
    auto items = line.split(',');
    if (items.size() < 3) {
      qWarning() << "[Similarity File] Line" << loaded_lines << "is too short!";
      continue;
    }
    QString spectrum_a = items[0].trimmed();
    QString spectrum_b = items[1].trimmed();
    double similarity = items[2].toDouble();
    if (spectrum_a.isEmpty() || spectrum_b.isEmpty()) {
      qWarning() << "[Similarity File] Line" << loaded_lines
                 << "content is incorrect!";
      continue;
    }

    spectrum_to_similarity_[spectrum_a][spectrum_b] = similarity;
    spectrum_to_similarity_[spectrum_b][spectrum_a] = similarity;

    qInfo() << "[SimilarityFile] Line" << loaded_lines << "loaded."
            << spectrum_a << "<->" << spectrum_b << "similarity:" << similarity;
    ++available_lines;
  }

  emit SimilarityFileLoaded(true, filename, available_lines);
  similarity_file_loaded_ = true;
  return true;
}

void StainingSchemeGenerator::SelectAntibody(const QString &antibody) {
  selected_antibodies_.insert(antibody);
  available_antibodies_.remove(antibody);
  emit SelectedAntibodiesRefreshed(selected_antibodies_.values());
  emit AvailableAntibodiesRefreshed(available_antibodies_.values());
  qDebug() << "Selected antibody:" << antibody;
}

void StainingSchemeGenerator::DeselectAntibody(const QString &antibody) {
  selected_antibodies_.remove(antibody);
  available_antibodies_.insert(antibody);
  emit SelectedAntibodiesRefreshed(selected_antibodies_.values());
  emit AvailableAntibodiesRefreshed(available_antibodies_.values());
  qDebug() << "Deselected antibody:" << antibody;
}

void StainingSchemeGenerator::GenerateSchemes() {
  StainingSchemeResultList scheme_result_list;
  QStringList selected_antibodies_list(selected_antibodies_.begin(),
                                       selected_antibodies_.end());
  QSet<QString> used_spectrum;
  StainingSchemeResult cur_result;
  cur_result.staining_groups.resize(selected_antibodies_list.size());

  SchemeBacktrace(selected_antibodies_list, used_spectrum, scheme_result_list,
                  cur_result, 0);

  // Sort schemes in ascending order by similarity
  std::sort(scheme_result_list.begin(), scheme_result_list.end(),
            [](const StainingSchemeResult &a, const StainingSchemeResult &b) {
              return a.similarity < b.similarity;
            });

  emit SchemesGenerated(scheme_result_list);
}

void StainingSchemeGenerator::SchemeBacktrace(
    const QStringList &selected_antibodies, QSet<QString> &used_spectrum,
    StainingSchemeResultList &result_list, StainingSchemeResult &cur_result,
    int cur_index) {
  // Stop recursive backtrace and collect result
  if (cur_index == selected_antibodies.size()) {
    if (similarity_file_loaded_) {
      CalculateSimilarity(cur_result);
    }
    result_list.append(cur_result);
    return;
  }

  const auto &antibody = selected_antibodies[cur_index];
  const auto &spectrum_to_channels = antibody_spectrum_to_channels_[antibody];
  cur_result.staining_groups[cur_index].antibody = antibody;

  for (auto it = spectrum_to_channels.constBegin();
       it != spectrum_to_channels.constEnd(); ++it) {
    const auto &spectrum = it.key();
    auto channels = it.value();
    for (const auto &channel : channels) {
      if (used_spectrum.contains(spectrum)) {
        continue;
      }

      cur_result.staining_groups[cur_index].spectrum = spectrum;
      cur_result.staining_groups[cur_index].channel = channel;
      used_spectrum.insert(spectrum);

      SchemeBacktrace(selected_antibodies, used_spectrum, result_list,
                      cur_result, cur_index + 1);

      used_spectrum.remove(spectrum);
    }
  }
}
void StainingSchemeGenerator::CalculateSimilarity(
    StainingSchemeResult &scheme) {
  const auto &staining_groups = scheme.staining_groups;
  auto n = staining_groups.size();
  double max_similarity = -1;
  for (qsizetype i = 0; i < n; ++i) {
    for (qsizetype j = i + 1; j < n; ++j) {
      auto spectrum_a = staining_groups[i].spectrum;
      auto spectrum_b = staining_groups[j].spectrum;
      if (!spectrum_to_similarity_.contains(spectrum_a) &&
          spectrum_to_similarity_[spectrum_a].contains(spectrum_b)) {
        qWarning() << "[CalculateSimilarity] similarity of" << spectrum_a
                   << "<->" << spectrum_b << "is lost.";
        continue;
      }
      max_similarity =
          qMax(max_similarity, spectrum_to_similarity_[spectrum_a][spectrum_b]);
    }
  }
  if (max_similarity != -1) {
    scheme.similarity = max_similarity;
  }
}
