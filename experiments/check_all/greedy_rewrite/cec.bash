for file in replace*.aig; do
    echo "Processing: $file"
    if [ -f ${file%.*}_cec.log ]; then
        echo "Skip: $file"
        continue
    fi
    ~/LSV/LSV-PA/abc -c "&read $file; &cec -m -v" > ${file%.*}_cec.log
    if [ $? -ne 0 ]; then
        echo "Error: $file"
    fi
done
