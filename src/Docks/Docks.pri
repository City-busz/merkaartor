INCLUDEPATH += $$MERKAARTOR_SRC_DIR/Docks
DEPENDPATH += $$MERKAARTOR_SRC_DIR/Docks

HEADERS += MDockAncestor.h \
    LayerDock.h \
    PropertiesDock.h \
    InfoDock.h \
    StyleDock.h \
    DirtyDock.h \
    FeaturesDock.h
SOURCES += MDockAncestor.cpp \
    PropertiesDock.cpp \
    InfoDock.cpp \
    LayerDock.cpp \
    DirtyDock.cpp \
    StyleDock.cpp \
    FeaturesDock.cpp
FORMS += DirtyDock.ui \
    StyleDock.ui \
    MinimumRelationProperties.ui \
    MinimumTrackPointProperties.ui \
    MinimumRoadProperties.ui \
    FeaturesDock.ui
