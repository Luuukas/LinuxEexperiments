dir=$1;shift
if [ -d $dir ]
then
    cd $dir
    for name in `ls $dir`
    do
        if [ -f $name ]
        then
            echo "${dir}/$name"
        else
            echo "invaild file name:${dir}/$name"
        fi
    done
else
    echo "bad directory name:${dir}"
fi