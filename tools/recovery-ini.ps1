function Set-RecoveryIniKey([string]$text,[string]$section,[string]$key,[string]$value){
 $eol=if($text.Contains("`r`n")){"`r`n"}else{"`n"}
 $sections=[regex]::Matches($text,'(?m)^\['+[regex]::Escape($section)+'\](?=\r?$)')
 if($sections.Count -ne 1){throw "Exactly one section required: $section"}
 $s=$sections[0];$start=$s.Index+$s.Length
 $next=[regex]::Match($text.Substring($start),'(?m)^\[')
 $end=if($next.Success){$start+$next.Index}else{$text.Length}
 $body=$text.Substring($start,$end-$start)
 $matches=[regex]::Matches($body,'(?m)^'+[regex]::Escape($key)+'=[^\r\n]*')
 if($matches.Count -gt 1){throw "Duplicate owned key: $key"}
 if($matches.Count){
  $m=$matches[0];return $text.Remove($start+$m.Index,$m.Length).Insert($start+$m.Index,"$key=$value")
 }
 return $text.Insert($start,$eol+"$key=$value")
}
