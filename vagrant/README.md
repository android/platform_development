Vagrant VMs
===========

Vagrant is a virtual machine manager: https://www.vagrantup.com/

Download and install Vagrant from their website. Note to Linux users: get it
from their website, not from your package manager. The package manager's version
is generally not up to date enough to work.

In this directory you will find virtual machines configured for testing in
Android. These machines will automatically sync their platform's out directory
to `C:\vagrant`.

The VM images are provided by Microsoft for testing (thanks, Microsoft!):
https://developer.microsoft.com/en-us/microsoft-edge/tools/vms/linux/.

Using Vagrant for Testing
-------------------------

```bash
cd development/vagrant/win32
vagrant up # Start the VM.
vagrant rdp # Open remote desktop connected to the VM.
```
