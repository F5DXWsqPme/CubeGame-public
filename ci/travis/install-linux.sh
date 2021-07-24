mkdir vk
cd vk
wget https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.gz
tar -xf archive.tar.gz

for file in . do                    
  tar -xf "$file"
done

for dir in . do                    
  if [ -d "$dir" ] then
    cd "$dir" 
  fi
done