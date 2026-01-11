# hycov (Maintained Fork)

A Hyprland overview mode plugin, a new tiling WM workflow.  

> [!NOTE]
> **This is an actively maintained fork** of the original [hycov by DreamMaoMao](https://github.com/DreamMaoMao/hycov), which was archived in July 2024.
> 
> All credit for the original plugin goes to **DreamMaoMao** - this fork simply updates it to work with newer Hyprland versions.

> [!WARNING]
> **Use at your own risk!** Not all original features have been tested with the updated code. 
> The core functionality (overview toggle, click to select, right-click to close) works, but some config options and advanced features may be broken or behave unexpectedly.
> Please report issues if you find something that doesn't work.

### Fork Status

| Feature | Status |
|---------|--------|
| Hyprland v0.53.0+ | Working |
| Overview toggle | Working |
| Click to select window | Working |
| Right-click to close | Working |
| Multi-monitor | Working |
| Auto-exit on window close | Working |
| Gesture support (touchpad) | Working |
| Alt-release exit | Experimental |
| Hotarea (corner trigger) | Removed (caused input issues) |

### What can it do?
- Hycov can tile all of your windows in a single workspace via grid layout.

- After quitting the overview mode, hycov can perfectly recover a window's previous state (fullscreen, floating, size, positon, etc.)

- Hycov supports a variety of trigger methods, such as touchpad gestures and keyboard shortcuts.

- Supports multiple monitors.

- You can change the way that Hycov focuses a window, whether directional or cyclical. (single-shortcut)

https://github.com/DreamMaoMao/hycov/assets/30348075/59121362-21a8-4143-be95-72ce79ee8e95


Anyone is welcome to fork. If you end up improving the plugin, please let me know, and I'll be happy to use your fork.

### Manual Installation

> [!NOTE]
> 1. After Hycov is installed, you may need to logout and log back in.
> 2. This fork targets **Hyprland v0.53.0+**. For older Hyprland versions, use the [original repo](https://github.com/DreamMaoMao/hycov). or another fork if the original repo does not support ur Hyprland version.

##### Using make (recommended for this fork):

```shell
git clone https://github.com/ernestoCruz05/hycov.git
cd hycov
make all
sudo cp build/libhycov.so /usr/lib/
```

##### Using meson and ninja:

```shell
git clone https://github.com/ernestoCruz05/hycov.git
cd hycov
sudo meson setup build --prefix=/usr
sudo ninja -C build
sudo ninja -C build install # `libhycov.so` path: /usr/lib/libhycov.so
```

##### Using CMake:

```shell
git clone https://github.com/ernestoCruz05/hycov.git
cd hycov
bash install.sh # `libhycov.so` path: /usr/lib/libhycov.so
```

##### Using hyprpm:

```shell
hyprpm update
hyprpm add https://github.com/ernestoCruz05/hycov
hyprpm enable hycov
```

### Usage (hyprland.conf)

```conf
# When entering overview mode, you can use left-button to jump, right-button to kill or use keybind

#  If you are installing hycov with hyprpm, you should comment out this 
plugin = /usr/lib/libhycov.so

# If you are installing hycov by manually compile , you should comment out this 
exec-once = hyprpm reload

# bind key to toggle overview (normal)
bind = ALT,tab,hycov:toggleoverview

# bind key to toggle overview (force mode, not affected by `only_active_workspace` or `only_active_monitor`)
bind = ALT,grave,hycov:toggleoverview,forceall #grave key is the '~' key

# bind key to toggle overview (force mode, not affected by `only_active_workspace` or `only_active_monitor`)
bind = ALT,c,hycov:toggleoverview,onlycurrentworkspace

# bind key to toggle overview (shows all windows in one monitor, not affected by `only_active_workspace` or `only_active_monitor`)
bind = ALT,g,hycov:toggleoverview,forceallinone 

# The key binding for directional switch mode.
# Calculate the window closest to the direction to switch focus.
# This keybind is applicable not only to the overview, but also to the general layout.
bind=ALT,left,hycov:movefocus,l
bind=ALT,right,hycov:movefocus,r
bind=ALT,up,hycov:movefocus,u
bind=ALT,down,hycov:movefocus,d

# if you want that focusmove can cross monitor, use this
bind=ALT,left,hycov:movefocus,leftcross
bind=ALT,right,hycov:movefocus,rightcross
bind=ALT,up,hycov:movefocus,upcross
bind=ALT,down,hycov:movefocus,downcross

plugin {
    hycov {
        # Gaps and appearance
        overview_gappo = 60 # gaps width from screen edge
        overview_gappi = 24 # gaps width from clients
        
        # Mouse click actions
        enable_click_action = 1 # enable mouse left button jump and right button kill in overview mode
        click_in_cursor = 1 # when click to jump, find target window by cursor position
        
        # Gesture settings (touchpad)
        enable_gesture = 0 # enable touchpad gesture to toggle overview
        swipe_fingers = 4 # number of fingers for gesture (3-4 recommended)
        move_focus_distance = 100 # swipe distance to move focus in overview
        
        # Behavior settings
        auto_exit = 1 # enable auto exit when no client in overview
        auto_fullscreen = 0 # auto make active window maximize after exit overview
        only_active_workspace = 0 # only overview the active workspace
        only_active_monitor = 0 # only overview the active monitor
        show_special = 0 # show windows in special workspace in overview
        raise_float_to_top = 1 # raise floating windows to top after leaving overview
        
        # Alt-release mode (experimental)
        enable_alt_release_exit = 0 # alt switch mode, see readme for detail
        alt_replace_key = Alt_L # key to detect for alt-release exit
        alt_toggle_auto_next = 0 # auto focus next window when toggle overview in alt switch mode
        
        # Advanced settings (usually don't need to change)
        height_of_titlebar = 0 # height deviation of title bar height
    }
}

```

# suggested additional configuration
- when `auto_fullscreen=1` is set, you can also set the border color to mark the maximize state and bind key to control fullscreen maximize state.
```
windowrulev2 = bordercolor rgb(158833),fullscreen:1 # set bordercolor to green if window is fullscreen maximize
# toggle fullscreen maximize
bind = ALT,a,fullscreen,1
```
<details>
<summary>detail video</summary>

https://github.com/DreamMaoMao/hycov/assets/30348075/15ba36c2-1782-4ae0-8ac1-d0ca98e01e0f

</details>


- if you use the `hyprland/workspaces` module in waybar,you should change field {id} to {name}. It will let you know you are in overview mode.
```
"hyprland/workspaces": {
    "format": "{name}",
    "on-click":"activate",
},
```

<details>
<summary>detail picture</summary>

![image](https://github.com/DreamMaoMao/hycov/assets/30348075/332f4025-20c1-4a44-853b-1b5264df986e)
![image](https://github.com/DreamMaoMao/hycov/assets/30348075/500d9fd7-299b-48bc-ab72-146f263044a5)

</details>


# Alt switch mode
```conf
enable_alt_release_exit = 1
alt_toggle_auto_next = 0 # auto focus next window when enter overview in alt mode
# alt_replace_key = Alt_L # If your MainKey of toggleoverview is ALt, you can ignore it
```
## operation
such as `alt + tab`:

- 1.`alt + tab` will enter overview when you not in overview(please hold alt,don't make it release)

- 2.`alt + tab` will switch window focus circularly when you in overview. (please hold alt,don't make it release)

- 3.when you release `alt` , it will auto exit overview.

<details>
<summary> If you don't want to use `alt` as MainKey in alt mode</summary>

such as use `super` to repalce `alt`
- 1.bind toggleoverview
```
bind = SUPER,tab,hycov:toggleoverview
```
- 2.use `alt_replace_key` to specify what is the detection key on release.
```
# use keyname
alt_replace_key = Super_L # Alt_L,Alt_R,Super_L,Super_R,Control_L,Control_R,Shift_L,Shift_R

# use keycode
alt_replace_key = code:133 # use `xev` command to get keycode
```

</details>

> [!WARNING]
> **I do not use NixOS personally!** The Nix derivation below is community-maintained. 
> If it breaks due to Hyprland updates, please feel free to submit a Pull Request to fix it.
> I will do my best to update the plugin everytime the API changes.

### NixOS with home—manager

```nix
# flake.nix

{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    hyprland.url = "github:hyprwm/Hyprland";

    hycov={
      url = "github:ernestoCruz05/hycov"; # Use this fork for Hyprland v0.53+
      inputs.hyprland.follows = "hyprland";
    };
  };

  outputs = { nixpkgs, home-manager, hyprland, hycov, ... }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      homeConfigurations."user@hostname" = home-manager.lib.homeManagerConfiguration {
        pkgs = nixpkgs.legacyPackages.x86_64-linux;

        modules = [
          hyprland.homeManagerModules.default
          {
            wayland.windowManager.hyprland = {
              enable = true;
              package = hyprland.packages."${pkgs.system}".hyprland;
              plugins = [
                hycov.packages.${pkgs.system}.hycov
              ];
              extraConfig = ''
                bind = ALT,tab,hycov:toggleoverview
                bind=ALT,left,hycov:movefocus,l
                bind=ALT,right,hycov:movefocus,r
                bind=ALT,up,hycov:movefocus,u
                bind=ALT,down,hycov:movefocus,d

                plugin {
                    hycov {
                      overview_gappo = 60 #gaps width from screen
                      overview_gappi = 24 #gaps width from clients
                      enable_click_action = 1 # left-click to select, right-click to close
                      enable_gesture = 1 # enable touchpad gestures
                      swipe_fingers = 4 # 4-finger swipe to toggle
                    }
                }
              '' + ''
                # your othor config
              '';
            };
          }
          # ...
        ];
      };
    };
}
```
## Frequently Asked Questions

- **Plugin not loading or config not working**
```
Try logging out and logging back in after first installation.
```

- **The numbers on the waybar are confused**
```
Change the {id} field in hyprland/workspace to {name}
```

- **Unable to load / plugin crashes**
```
Make sure you rebuild hycov after updating Hyprland.
This fork is built for Hyprland v0.53.0+.
```

- **Mouse clicks not working in overview**
```
Make sure enable_click_action = 1 is set in your config.
```

- **Gestures not working**
```
Make sure enable_gesture = 1 is set and swipe_fingers matches the 
number of fingers you're using on your touchpad.
```

