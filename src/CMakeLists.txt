PROJECT(Mastermind)

SET(SRC_LIST CodeBreaker.cpp Engine.cpp ObviousStrategy.cpp Codeword.cpp OptimalCodeBreaker.cpp ColorEquivalence.cpp Generation.cpp StrategyTree.cpp Compare.cpp Interactive.cpp Test.cpp ConstraintEquivalence.cpp Main.cpp DummyEquivalenceFilter.cpp Mask.cpp)

SET(CMAKE_CXX_FLAGS "-std=c++0x -Wall -O2 -fopenmp")

ADD_EXECUTABLE(Mastermind ${SRC_LIST})
