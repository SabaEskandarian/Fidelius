## Fidelius: Protecting User Secrets from Compromised Browsers 

 Users regularly enter sensitive data, such as passwords, credit card numbers, or tax information, into the browser window. While modern browsers provide powerful client-side privacy measures to protect this data, none of these defenses prevent a browser compromised by malware from stealing it. Fidelius is a new architecture that uses trusted hardware enclaves integrated into the browser to enable protection of user secrets during web browsing sessions, even if the entire underlying browser and OS are fully controlled by a malicious attacker.

As part of this project, we develop the first open source system that provides a trusted path from input and output peripherals to a hardware enclave with no reliance on additional hypervisor security assumptions. These components may be of independent interest and useful to future projects. 

This repository contains our prototype implementation of Fidelius, including the Web Enclave code and code to run on our keyboard/display dongles. 
Details of the Fidelius architecture and security properties can be found in our paper, available at https://arxiv.org/pdf/1809.04774.pdf. 
