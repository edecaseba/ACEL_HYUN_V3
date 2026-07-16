#!/usr/bin/env python3
"""
Skill: validate_orchestration
Valida la orquestación completa de agentes: archivos, permisos, alineación y fallback.
"""

import json
import os
from pathlib import Path

def validate_orchestration(scope="all"):
    """
    Valida la orquestación de agentes según el alcance especificado.
    
    Args:
        scope (str): Alcance de validación - 'all', 'files', 'permissions', 'fallback', 'alignment'
    
    Returns:
        dict: Resultados de validación con estado y hallazgos
    """
    
    base_path = Path("/home/seba/Documentos/proyectos-personales/ACEL_HYUN_V3/ACEL_HYUN_V3")
    results = {
        "status": "PASS",
        "findings": [],
        "errors": [],
        "warnings": []
    }
    
    if scope in ["all", "files"]:
        # Validar archivos críticos
        critical_files = [
            "ai/hardware_target.json",
            "AGENTS.md",
            ".opencode/agents/orchestrator.md",
            ".opencode/agents/explore.md"
        ]
        
        for file_path in critical_files:
            full_path = base_path / file_path
            if not full_path.exists():
                results["errors"].append(f"Archivo faltante: {file_path}")
                results["status"] = "FAIL"
            else:
                # Verificar contenido básico
                try:
                    with open(full_path, 'r', encoding='utf-8') as f:
                        content = f.read()
                        if len(content.strip()) < 10:
                            results["warnings"].append(f"Archivo {file_path} parece vacío o muy corto")
                except Exception as e:
                    results["errors"].append(f"Error leyendo {file_path}: {str(e)}")
                    results["status"] = "FAIL"
    
    if scope in ["all", "permissions"]:
        # Validar permisos de agentes
        opencode_path = base_path / ".opencode"
        if (opencode_path / "agents").exists():
            for agent_file in (opencode_path / "agents").glob("*.md"):
                try:
                    with open(agent_file, 'r', encoding='utf-8') as f:
                        content = f.read()
                        # Extraer nombre del agente del contenido del archivo
                        if "description:" in content:
                            # Extraer nombre del agente del contenido
                            lines = content.split('\n')
                            for line in lines:
                                if line.strip().startswith('description:'):
                                    desc = line.strip()
                                    if 'Orquestador' in desc:
                                        agent_name = "orchestrator"
                                    elif 'Arquitecto' in desc:
                                        agent_name = "planner"
                                    elif 'Programador' in desc:
                                        agent_name = "coder"
                                    elif 'Revisor' in desc:
                                        agent_name = "reviewer"
                                    elif 'QA' in desc:
                                        agent_name = "tester"
                                    elif 'Documentador' in desc:
                                        agent_name = "documenter"
                                    elif 'Explorador' in desc:
                                        agent_name = "explore"
                                    elif 'Respaldo' in desc:
                                        agent_name = "planner_fallback"
                                    else:
                                        agent_name = agent_file.stem
                                    break
                        else:
                            agent_name = agent_file.stem
                        
                        if 'read: allow' not in content:
                            results["warnings"].append(f"Agente {agent_name} puede no tener permisos de lectura")
                except Exception as e:
                    results["errors"].append(f"Error verificando permisos de {agent_file.name}: {str(e)}")
    
    if scope in ["all", "fallback"]:
        # Validar configuración de fallback
        opencode_json_path = base_path / "opencode.json"
        if opencode_json_path.exists():
            try:
                with open(opencode_json_path, 'r', encoding='utf-8') as f:
                    config = json.load(f)
                
                # Verificar si existe orchestrator_fallback
                if "orchestrator_fallback" not in config["agent"]:
                    results["warnings"].append("orchestrator_fallback no encontrado en opencode.json")
                
                # Verificar prioridad de fallback
                if "planner_fallback" not in config["agent"]:
                    results["warnings"].append("planner_fallback no encontrado en opencode.json")
                    
            except Exception as e:
                results["errors"].append(f"Error validando fallback en opencode.json: {str(e)}")
                results["status"] = "FAIL"
    
    if scope in ["all", "alignment"]:
        # Validar alineación AGENTS.md ↔ opencode.json
        agents_md_path = base_path / "AGENTS.md"
        opencode_json_path = base_path / "opencode.json"
        
        if agents_md_path.exists() and opencode_json_path.exists():
            try:
                with open(agents_md_path, 'r', encoding='utf-8') as f:
                    agents_content = f.read()
                
                with open(opencode_json_path, 'r', encoding='utf-8') as f:
                    config = json.load(f)
                
                # Verificar que AGENTS.md contiene protocolo de autorización
                if "Protocolo del Equipo (con autorización del usuario)" not in agents_content:
                    results["errors"].append("AGENTS.md no contiene protocolo de autorización")
                    results["status"] = "FAIL"
                
                # Verificar que hay al menos 8 agentes definidos
                agent_count = len(config["agent"])
                if agent_count < 8:
                    results["warnings"].append(f"Solo {agent_count} agentes definidos, se esperan al menos 8")
                    
            except Exception as e:
                results["errors"].append(f"Error validando alineación: {str(e)}")
                results["status"] = "FAIL"
    
    # Resumen final
    if results["status"] == "PASS":
        results["findings"].append("✅ Validación completada exitosamente")
        if results["warnings"]:
            results["findings"].append(f"⚠️ {len(results['warnings'])} advertencias encontradas")
    else:
        results["findings"].append(f"❌ {len(results['errors'])} errores críticos encontrados")
    
    return results

if __name__ == "__main__":
    # Ejemplo de uso
    import sys
    scope = sys.argv[1] if len(sys.argv) > 1 else "all"
    
    result = validate_orchestration(scope)
    
    print("=== VALIDACIÓN DE ORQUESTACIÓN ===")
    print(f"Estado: {result['status']}")
    print("\nHallazgos:")
    for finding in result['findings']:
        print(f"  {finding}")
    
    if result['errors']:
        print("\nErrores:")
        for error in result['errors']:
            print(f"  ❌ {error}")
    
    if result['warnings']:
        print("\nAdvertencias:")
        for warning in result['warnings']:
            print(f"  ⚠️ {warning}")
    
    exit(0 if result['status'] == 'PASS' else 1)