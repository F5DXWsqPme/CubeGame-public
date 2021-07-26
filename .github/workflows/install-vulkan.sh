mkdir vk
cd vk

wget --no-verbose https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.gz

for file in *
do
  echo "$file in vk"
  if [ -f "$file" ]
  then
    echo "$file downloaded"
    tar -xzf "$file"
    break
  fi
done

for dir in *
do
  echo "$dir in vk"
  if [ -d "$dir" ]
  then
    echo "$dir extracted"
    cd "$dir"
    break
  fi
done

./vulkansdk