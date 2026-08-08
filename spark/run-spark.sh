#!/bin/bash

/opt/spark/bin/spark-submit --master spark://dis-compute-1-of-3:7077 --conf spark.executor.instances=3  --conf spark.executor.cores=2 $@

