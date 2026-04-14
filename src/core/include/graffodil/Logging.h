#pragma once

#include <QLoggingCategory>

namespace Graffodil {

/// Logging category for library warnings and diagnostics.
/// Consumers can silence these via:
///   QLoggingCategory::setFilterRules("graffodil.warning=false");
/// or the environment variable:
///   QT_LOGGING_RULES="graffodil.warning=false"
Q_DECLARE_LOGGING_CATEGORY(lcGraffodil)

} // namespace Graffodil
