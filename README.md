# hycov-rev
A Hyprland overview mode script


### What can it do?
- Hycov-rev can tile all of your windows in a single workspace via dispatcher.

- After quitting the overview mode, hycov can perfectly recover a window's previous state (fullscreen, floating, size, positon, etc.)

- Hycov-rev supports a variety of trigger methods, such as touch pad gestures, hot corners, and keyboard shortcuts.


Fork from：https://github.com/DreamMaoMao/hycov/


Anyone is welcome to fork. If you end up improving the thing, please let me know, and I'll be happy to use your fork.

### Installation
```
git clone https://github.com/gfhdhytghd/hycov-rev.git
mkdir-p  ~/.config/hypr/scripts/
cp ./hycov-rev/hycov.sh ~/.config/hypr/scripts/
```
### Usage (hyprland.conf)

```conf
gesture = 4, up, dispatcher,exec, ~/.config/hypr/scripts/hycov.sh --ovon
gesture = 4, down, dispatcher,exec, ~/.config/hypr/scripts/hycov.sh --ovoff

```
<details>
<summary>detail video</summary>


https://github.com/user-attachments/assets/0fa5921b-99ab-4646-8c1a-ddbd74c0ba82



</details>

for more inforemation, read the code.
