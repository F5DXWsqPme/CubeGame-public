mkdir vk
cd vk

wget https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.gz

for file in ~/vk
do
  echo "$file in vk"
  if [ -f "$file" ]
  then
    echo "$file downloaded"
    tar -xzf "$file"
    break
  fi
done

for dir in ~/vk
do
  echo "$dir in vk"
  if [ -d "$dir" ]
  then
    echo "$dir extracted"
    cd "$dir"
    break
  fi
done