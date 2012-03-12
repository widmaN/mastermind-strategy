SET(SRC_LIST CodeBreaker.cpp Engine.cpp ObviousStrategy.cpp Codeword.cpp OptimalCodeBreaker.cpp ColorEquivalence.cpp Generation.cpp StrategyTree.cpp Compare.cpp Interactive.cpp Test.cpp ConstraintEquivalence.cpp Main.cpp DummyEquivalenceFilter.cpp Mask.cpp)

SET(CMAKE_CXX_FLAGS "-g -DNDEBUG -std=c++0x -Wall -Wextra -O2 -fopenmp")

ADD_EXECUTABLE(Mastermind ${SRC_LIST})
