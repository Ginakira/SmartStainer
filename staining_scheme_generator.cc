//
// Created by ginak on 2023/6/18.
//

#include <QString>
#include <QFile>
#include <QDebug>

#include "staining_scheme_generator.h"

StainingSchemeGenerator::StainingSchemeGenerator(QObject *parent,
                                                 QString filename)
    : QObject(parent), filename_(std::move(filename)) {
}

bool StainingSchemeGenerator::LoadFile() {
  QFile file(filename_);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    emit FileLoaded(false, "", -1);
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
      qWarning() << "Line" << loaded_lines << "is too short!";
      continue;
    }
    QString spectrum = items[0].trimmed();
    QString fluorescence_channel = items[1].trimmed();
    QString anti_body = items[2].trimmed();

    if (spectrum.isEmpty() || fluorescence_channel.isEmpty()
        || anti_body.isEmpty()) {
      qWarning() << "Line" << loaded_lines << "content is incorrect!";
      continue;
    }

    antibody_spectrum_to_channels_[anti_body][spectrum]
        .insert(fluorescence_channel);
    available_antibodies_.insert(anti_body);

    qDebug() << "Line" << loaded_lines << "loaded. spectrum:" << spectrum
             << "fluorescence channel:"
             << fluorescence_channel << "anti body:" << anti_body;
    ++available_lines;
  }


  emit FileLoaded(true, filename_, available_lines);
  emit AvailableAntibodiesRefreshed(available_antibodies_.values());
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
  QStringList selected_antibodies_list
      (selected_antibodies_.begin(), selected_antibodies_.end());
  QSet<QPair<QString, QString>> used_spectrum_channel;
  StainingSchemeResult cur_result(selected_antibodies_list.size());

  SchemeBacktrace(selected_antibodies_list,
                  used_spectrum_channel,
                  scheme_result_list,
                  cur_result, 0);

  emit SchemesGenerated(scheme_result_list);
}

void StainingSchemeGenerator::SchemeBacktrace(const QStringList &selected_antibodies,
                                              QSet<QPair<QString,
                                                         QString>> &used_spectrum_channel,
                                              StainingSchemeResultList &result_list,
                                              StainingSchemeResult &cur_result,
                                              int cur_index) {
  if (cur_index == selected_antibodies.size()) {
    result_list.append(cur_result);
    return;
  }
  const auto &antibody = selected_antibodies[cur_index];
  const auto &spectrum_to_channels = antibody_spectrum_to_channels_[antibody];
  cur_result[cur_index].antibody = antibody;

  for (auto it = spectrum_to_channels.constBegin();
       it != spectrum_to_channels.constEnd(); ++it) {
    auto spectrum = it.key();
    auto channels = it.value();
    for (const auto &channel : channels) {
      auto spectrum_channel = qMakePair(spectrum, channel);
      if (used_spectrum_channel.contains(spectrum_channel)) {
        continue;
      }

      cur_result[cur_index].spectrum = spectrum;
      cur_result[cur_index].channel = channel;
      used_spectrum_channel.insert(spectrum_channel);

      SchemeBacktrace(selected_antibodies,
                      used_spectrum_channel,
                      result_list,
                      cur_result,
                      cur_index + 1);

      used_spectrum_channel.remove(spectrum_channel);
    }
  }
}

StainingSchemeGenerator::~StainingSchemeGenerator() = default;
