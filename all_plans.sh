build='cmake-build-release'
plan_path='../benchmarks/join-order-benchmark/plans/preprocessed/'
exec='./main'
suffix='.json'

cd $build
for n in 1 2 4 8 16 32
do
  for p in $(ls $plan_path)
    do
      $exec ${p%$suffix} $n
    done
done
cd ..