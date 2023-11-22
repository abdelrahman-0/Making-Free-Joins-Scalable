build='cmake-build-release'
exec='./benchmark_indexes'

cd $build
for n in 1 2 4 8 16 32
do
  $exec $n
done
cd ..
