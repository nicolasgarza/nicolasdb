cd build &&
cmake --build . &&
ctest --verbose -R LexerTest &&
cd ..
