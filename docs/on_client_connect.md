# Event Client Connect

## Steps
1. Load client config
2. If client doesnt' exists or is disabled, quit event.
3. Load client networks
4. Load others client networks
6. Summarize others client networks to routes
   with no gateway (equals gateway is VPN server) 
7. Create VPN client config
8. Add client networks to VPN client config
9. Add summarized routes to VPN client config
10. Build VPN client config
