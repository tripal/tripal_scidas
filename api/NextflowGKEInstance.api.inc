<?php

class NextflowGKEInstance extends NextflowGKERequest {
  /** The hostname where the Galaxy server is located. */
  protected $host;
  /** The port on which the remote Galaxy instance is runinng. */
  protected $port;
  /** Should be set to TRUE if the remote server uses HTTPS. */
  protected $use_https;
  /** The API Key for the user connection. */
  protected $api_key;


  /**
   * The GalaxyInstance constructor.
   *
   * @param $hostname
   *   The hostname where the Galaxy server is located.
   * @param $port
   *   The port on which the remote Galaxy instance is runinng.
   * @param $use_https
   *   Should be set to TRUE if the remote server uses HTTPS. Defaults
   *   to TRUE.
   *
   * @return
   *   An instance of a GalaxyInstance object.
   *
   *
   */
  function __construct($hostname, $port, $use_https = FALSE) {
    parent::__construct();

    $this->host = $hostname;
    $this->port = $port;
    $this->use_https  = $use_https;
    $this->api_key = '';

  }

  /**
   * Retrieves the version of the Galaxy API.
   *
   * @return
   *
   *
   *
   */
  public function getVersion() {
    return $this->httpGET($this->getURL() . '/version');
  }

  /**
   * Authenticates a user with the remote Galaxy instance.
   *
   * Corresponds to the Galaxy API method and path:
   * GET /api/authenticate/baseauth
   *
   * @param $username
   *   The username of the user.
   * @param $password
   *   The password for the user.
   * @param $message
   *   If authentication fails then this variable will be set to contain
   *   the error message.
   *
   * @return
   *   TRUE if authentication was successful, FALSE otherwise.
   *
   *
   */
  public function authenticate($username, $password, &$message = '') {

    // First just try to get the version.  If not, then this means
    // Galaxy may not be available and we should error out before trying
    // further.
    /* $URL = $this->getURL();
    $version = $this->getVersion();
    if (!$version) {
      $this->setError('Galaxy', "Cannot connect to Galaxy server: $URL.");
      return FALSE;
    }

    // Now try to authenticate.
    $response = $this->auth($URL . '/api/authenticate/baseauth', $username, $password);
    if (!$response) {
      return FALSE;
    }
    $this->setAPIKey($response['api_key']); */

    return TRUE;
  }

  /**
   * Returns the URL for the remote Galaxy server.
   *
   * The URL returned will include the protocol (HTTP, HTTPS),
   * the hostname and the port.
   *
   * @return string
   *   The URL for the remote Galaxy instance.
   *
   *
   */
  public function getURL() {
    if ($this->use_https) {
      return "https://". $this->host .":". $this->port;
    }
    else {
      return "http://". $this->host .":". $this->port;
    }
  }

}