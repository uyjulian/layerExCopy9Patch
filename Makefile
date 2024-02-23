
INCFLAGS += -Iexternal/simde

SOURCES += main.cpp
SOURCES += ComplexRect.cpp
SOURCES += ResampleImage.cpp
SOURCES += WeightFunctor.cpp
SOURCES += ResampleImageSSE2.cpp
SOURCES += x86simdutil.cpp
SOURCES += roundevenf.c
SOURCES += LayerBitmapUtility.cpp
SOURCES += tvpgl.cpp
SOURCES += RectItf.cpp
PROJECT_BASENAME = layerExCopy9Patch

RC_LEGALCOPYRIGHT ?= Copyright (C) 2023-2023 Julian Uy; See details of license at license.txt, or the source code location.

include external/ncbind/Rules.lib.make
