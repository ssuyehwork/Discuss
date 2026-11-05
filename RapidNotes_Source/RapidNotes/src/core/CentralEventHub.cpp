#include "CentralEventHub.h"

namespace RapidNotes {

CentralEventHub& CentralEventHub::instance() {
    static CentralEventHub s_instance;
    return s_instance;
}

CentralEventHub::CentralEventHub(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<RapidNotes::AppEvent>("RapidNotes::AppEvent");
}

void CentralEventHub::publishEvent(const AppEvent& event) {
    emit eventOccurred(event);
}

} // namespace RapidNotes
