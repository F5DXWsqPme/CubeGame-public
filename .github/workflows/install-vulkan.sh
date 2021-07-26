mkdir vk
cd vk

wget https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.gz

for file in .
do
  echo "$file in current directory"
  if [ -f "$file" ]
  then
    echo "$file downloaded"
    tar -xzf $file
  fi
done

for dir in .
do
  echo "$dir in current directory"
  if [ -d "$dir" ]
  then
    echo "$dir extracted"
    cd $dir
    break
  fi
done